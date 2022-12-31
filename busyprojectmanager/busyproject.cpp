/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Copyright (C) 2022 Rochus Keller (me@rochus-keller.ch) for LeanCreator
**
** This file is part of LeanCreator.
**
** $QT_BEGIN_LICENSE:LGPL21$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "busyproject.h"

#include "busybuildconfiguration.h"
#include "busylogsink.h"
#include "busyprojectfile.h"
#include "busyprojectmanager.h"
#include "busyprojectparser.h"
#include "busyprojectmanagerconstants.h"
#include "busynodes.h"

#include <core/documentmanager.h>
#include <core/icontext.h>
#include <core/id.h>
#include <core/icore.h>
#include <core/iversioncontrol.h>
#include <core/vcsmanager.h>
#include <core/messagemanager.h>
#include <core/progressmanager/progressmanager.h>
#include <cpptools/cppmodelmanager.h>
#include <projectexplorer/buildenvironmentwidget.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/headerpath.h>
#if 0
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/uicodemodelsupport.h>
#include <qmljstools/qmljsmodelmanager.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#endif
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <busy/busyapi.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QVariantMap>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace BusyProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// Constants:
// --------------------------------------------------------------------

static const char CONFIG_CPP_MODULE[] = "cpp";
static const char CONFIG_CXXFLAGS[] = "cxxFlags";
static const char CONFIG_CFLAGS[] = "cFlags";
static const char CONFIG_DEFINES[] = "defines";
static const char CONFIG_INCLUDEPATHS[] = "includePaths";
static const char CONFIG_SYSTEM_INCLUDEPATHS[] = "systemIncludePaths";
static const char CONFIG_FRAMEWORKPATHS[] = "frameworkPaths";
static const char CONFIG_SYSTEM_FRAMEWORKPATHS[] = "systemFrameworkPaths";
static const char CONFIG_PRECOMPILEDHEADER[] = "precompiledHeader";

// --------------------------------------------------------------------
// BusyProject:
// --------------------------------------------------------------------

BusyProject::BusyProject(BusyManager *manager, const QString &fileName) :
    m_manager(manager),
    m_projectName(QFileInfo(fileName).completeBaseName()),
    m_fileName(fileName),
    m_rootProjectNode(0),
    m_qbsProjectParser(0),
    m_qbsUpdateFutureInterface(0),
    m_parsingScheduled(false),
    m_cancelStatus(CancelStatusNone),
    m_currentBc(0)
{
    m_parsingDelay.setInterval(1000); // delay parsing by 1s.

    setId(Constants::PROJECT_ID);
    setProjectContext(Context(Constants::PROJECT_ID));
    setProjectLanguages(Context(ProjectExplorer::Constants::LANG_CXX));

    connect(this, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
            this, SLOT(changeActiveTarget(ProjectExplorer::Target*)));
    connect(this, SIGNAL(addedTarget(ProjectExplorer::Target*)),
            this, SLOT(targetWasAdded(ProjectExplorer::Target*)));
    connect(this, SIGNAL(environmentChanged()), this, SLOT(delayParsing()));

    connect(&m_parsingDelay, SIGNAL(timeout()), this, SLOT(startParsing()));

    updateDocuments(QSet<QString>() << fileName);

    // NOTE: BusyProjectNode does not use this as a parent!
    m_rootProjectNode = new BusyRootProjectNode(this); // needs documents to be initialized!
}

BusyProject::~BusyProject()
{
    m_codeModelFuture.cancel();
    delete m_qbsProjectParser;
    if (m_qbsUpdateFutureInterface) {
        m_qbsUpdateFutureInterface->reportCanceled();
        m_qbsUpdateFutureInterface->reportFinished();
        delete m_qbsUpdateFutureInterface;
        m_qbsUpdateFutureInterface = 0;
    }

    // Deleting the root node triggers a few things, make sure rootProjectNode
    // returns 0 already
    BusyProjectNode *root = m_rootProjectNode;
    m_rootProjectNode = 0;
    delete root;
}

QString BusyProject::displayName() const
{
    return m_projectName;
}

IDocument *BusyProject::document() const
{
    foreach (IDocument *doc, m_qbsDocuments) {
        if (doc->filePath().toString() == m_fileName)
            return doc;
    }
    QTC_ASSERT(false, return 0);
}

BusyManager *BusyProject::projectManager() const
{
    return m_manager;
}

ProjectNode *BusyProject::rootProjectNode() const
{
    return m_rootProjectNode;
}

static void collectFilesForProject(const busy::ProjectData &project, QSet<QString> &result)
{
    result.insert(project.location().filePath());
    foreach (const busy::ProductData &prd, project.products()) {
        foreach (const busy::GroupData &grp, prd.groups()) {
            foreach (const QString &file, grp.allFilePaths())
                result.insert(file);
            result.insert(grp.location().filePath());
        }
        result.insert(prd.location().filePath());
    }
    foreach (const busy::ProjectData &subProject, project.subProjects())
        collectFilesForProject(subProject, result);
}

QStringList BusyProject::files(Project::FilesMode fileMode) const
{
    Q_UNUSED(fileMode);
    if (!m_qbsProject.isValid() || isParsing())
        return QStringList();
    QSet<QString> result;
    collectFilesForProject(m_projectData, result);
    result.unite(m_qbsProject.buildSystemFiles());
    return result.toList();
}

bool BusyProject::isProjectEditable() const
{
    return m_qbsProject.isValid() && !isParsing() && !BuildManager::isBuilding();
}

class ChangeExpector
{
public:
    ChangeExpector(const QString &filePath, const QSet<IDocument *> &documents)
        : m_document(0)
    {
        foreach (IDocument * const doc, documents) {
            if (doc->filePath().toString() == filePath) {
                m_document = doc;
                break;
            }
        }
        QTC_ASSERT(m_document, return);
        DocumentManager::expectFileChange(filePath);
        m_wasInDocumentManager = DocumentManager::removeDocument(m_document);
        QTC_CHECK(m_wasInDocumentManager);
    }

    ~ChangeExpector()
    {
        QTC_ASSERT(m_document, return);
        DocumentManager::addDocument(m_document);
        DocumentManager::unexpectFileChange(m_document->filePath().toString());
    }

private:
    IDocument *m_document;
    bool m_wasInDocumentManager;
};

bool BusyProject::ensureWriteableBusyFile(const QString &file)
{
    // Ensure that the file is not read only
    QFileInfo fi(file);
    if (!fi.isWritable()) {
        // Try via vcs manager
        IVersionControl *versionControl =
            VcsManager::findVersionControlForDirectory(fi.absolutePath());
        if (!versionControl || !versionControl->vcsOpen(file)) {
            bool makeWritable = QFile::setPermissions(file, fi.permissions() | QFile::WriteUser);
            if (!makeWritable) {
                QMessageBox::warning(ICore::mainWindow(),
                                     tr("Failed!"),
                                     tr("Could not write project file %1.").arg(file));
                return false;
            }
        }
    }
    return true;
}

busy::GroupData BusyProject::reRetrieveGroupData(const busy::ProductData &oldProduct,
                                          const busy::GroupData &oldGroup)
{
    busy::GroupData newGroup;
    foreach (const busy::ProductData &pd, m_projectData.allProducts()) {
        if (uniqueProductName(pd) == uniqueProductName(oldProduct)) {
            foreach (const busy::GroupData &gd, pd.groups()) {
                if (gd.location() == oldGroup.location()) {
                    newGroup = gd;
                    break;
                }
            }
            break;
        }
    }
    QTC_CHECK(newGroup.isValid());
    return newGroup;
}

bool BusyProject::addFilesToProduct(BusyBaseProjectNode *node, const QStringList &filePaths,
        const busy::ProductData &productData, const busy::GroupData &groupData, QStringList *notAdded)
{
    QTC_ASSERT(m_qbsProject.isValid(), return false);
    QStringList allPaths = groupData.allFilePaths();
    const QString productFilePath = productData.location().filePath();
    ChangeExpector expector(productFilePath, m_qbsDocuments);
    ensureWriteableBusyFile(productFilePath);
    foreach (const QString &path, filePaths) {
        busy::ErrorInfo err = m_qbsProject.addFiles(productData, groupData, QStringList() << path);
        if (err.hasError()) {
            MessageManager::write(err.toString());
            *notAdded += path;
        } else {
            allPaths += path;
        }
    }
    if (notAdded->count() != filePaths.count()) {
        m_projectData = m_qbsProject.projectData();
        BusyGroupNode::setupFiles(node, reRetrieveGroupData(productData, groupData),
                                 allPaths, QFileInfo(productFilePath).absolutePath(), true);
        m_rootProjectNode->update();
        emit fileListChanged();
    }
    return notAdded->isEmpty();
}

bool BusyProject::removeFilesFromProduct(BusyBaseProjectNode *node, const QStringList &filePaths,
        const busy::ProductData &productData, const busy::GroupData &groupData,
        QStringList *notRemoved)
{
    QTC_ASSERT(m_qbsProject.isValid(), return false);
    QStringList allPaths = groupData.allFilePaths();
    const QString productFilePath = productData.location().filePath();
    ChangeExpector expector(productFilePath, m_qbsDocuments);
    ensureWriteableBusyFile(productFilePath);
    foreach (const QString &path, filePaths) {
        busy::ErrorInfo err
                = m_qbsProject.removeFiles(productData, groupData, QStringList() << path);
        if (err.hasError()) {
            MessageManager::write(err.toString());
            *notRemoved += path;
        } else {
            allPaths.removeOne(path);
        }
    }
    if (notRemoved->count() != filePaths.count()) {
        m_projectData = m_qbsProject.projectData();
        BusyGroupNode::setupFiles(node, reRetrieveGroupData(productData, groupData), allPaths,
                                 QFileInfo(productFilePath).absolutePath(), true);
        m_rootProjectNode->update();
        emit fileListChanged();
    }
    return notRemoved->isEmpty();
}

bool BusyProject::renameFileInProduct(BusyBaseProjectNode *node, const QString &oldPath,
        const QString &newPath, const busy::ProductData &productData,
        const busy::GroupData &groupData)
{
    if (newPath.isEmpty())
        return false;
    QStringList dummy;
    if (!removeFilesFromProduct(node, QStringList() << oldPath, productData, groupData, &dummy))
        return false;
    busy::ProductData newProductData;
    foreach (const busy::ProductData &p, m_projectData.allProducts()) {
        if (uniqueProductName(p) == uniqueProductName(productData)) {
            newProductData = p;
            break;
        }
    }
    if (!newProductData.isValid())
        return false;
    busy::GroupData newGroupData;
    foreach (const busy::GroupData &g, newProductData.groups()) {
        if (g.name() == groupData.name()) {
            newGroupData = g;
            break;
        }
    }
    if (!newGroupData.isValid())
        return false;

    return addFilesToProduct(node, QStringList() << newPath, newProductData, newGroupData, &dummy);
}

void BusyProject::invalidate()
{
    prepareForParsing();
}

busy::BuildJob *BusyProject::build(const busy::BuildOptions &opts, QStringList productNames,
                                 QString &error)
{
    QTC_ASSERT(busyProject().isValid(), return 0);
    QTC_ASSERT(!isParsing(), return 0);

    if (productNames.isEmpty())
        return busyProject().buildAllProducts(opts);

    QList<busy::ProductData> products;
    foreach (const QString &productName, productNames) {
        bool found = false;
        foreach (const busy::ProductData &data, busyProjectData().allProducts()) {
            if (uniqueProductName(data) == productName) {
                found = true;
                products.append(data);
                break;
            }
        }
        if (!found) {
            error = tr("Cannot build: Selected products do not exist anymore.");
            return 0;
        }
    }

    return busyProject().buildSomeProducts(products, opts);
}

busy::CleanJob *BusyProject::clean(const busy::CleanOptions &opts)
{
    if (!busyProject().isValid())
        return 0;
    return busyProject().cleanAllProducts(opts);
}

busy::InstallJob *BusyProject::install(const busy::InstallOptions &opts)
{
    if (!busyProject().isValid())
        return 0;
    return busyProject().installAllProducts(opts);
}

QString BusyProject::profileForTarget(const Target *t) const
{
    return m_manager->profileForKit(t->kit());
}

bool BusyProject::isParsing() const
{
    return m_qbsUpdateFutureInterface;
}

bool BusyProject::hasParseResult() const
{
    return busyProject().isValid();
}

busy::Project BusyProject::busyProject() const
{
    return m_qbsProject;
}

busy::ProjectData BusyProject::busyProjectData() const
{
    return m_projectData;
}

bool BusyProject::needsSpecialDeployment() const
{
    return true;
}

void BusyProject::handleBusyParsingDone(bool success)
{
    QTC_ASSERT(m_qbsProjectParser, return);

    const CancelStatus cancelStatus = m_cancelStatus;
    m_cancelStatus = CancelStatusNone;

    // Start a new one parse operation right away, ignoring the old result.
    if (cancelStatus == CancelStatusCancelingForReparse) {
        m_qbsProjectParser->deleteLater();
        m_qbsProjectParser = 0;
        parseCurrentBuildConfiguration();
        return;
    }

    generateErrors(m_qbsProjectParser->error());

    bool dataChanged = false;
    if (success) {
        m_qbsProject = m_qbsProjectParser->busyProject();
        const busy::ProjectData &projectData = m_qbsProject.projectData();
        QTC_CHECK(m_qbsProject.isValid());

        if (projectData != m_projectData) {
            m_projectData = projectData;
            m_rootProjectNode->update();

            updateDocuments(m_qbsProject.isValid()
                            ? m_qbsProject.buildSystemFiles() : QSet<QString>() << m_fileName);
            dataChanged = true;
        }
    } else {
        m_qbsUpdateFutureInterface->reportCanceled();
    }

    m_qbsProjectParser->deleteLater();
    m_qbsProjectParser = 0;

    if (m_qbsUpdateFutureInterface) {
        m_qbsUpdateFutureInterface->reportFinished();
        delete m_qbsUpdateFutureInterface;
        m_qbsUpdateFutureInterface = 0;
    }

    if (dataChanged) { // Do this now when isParsing() is false!
        updateCppCodeModel();
        updateQmlJsCodeModel();
        updateBuildTargetData();

        emit fileListChanged();
    }
    emit projectParsingDone(success);
}

void BusyProject::targetWasAdded(Target *t)
{
    connect(t, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
            this, SLOT(delayParsing()));
    connect(t, SIGNAL(buildDirectoryChanged()), this, SLOT(delayParsing()));
}

void BusyProject::changeActiveTarget(Target *t)
{
    BuildConfiguration *bc = 0;
    if (t && t->kit())
        bc = t->activeBuildConfiguration();
    buildConfigurationChanged(bc);
}

void BusyProject::buildConfigurationChanged(BuildConfiguration *bc)
{
    if (m_currentBc)
        disconnect(m_currentBc, SIGNAL(busyConfigurationChanged()), this, SLOT(delayParsing()));

    m_currentBc = qobject_cast<BusyBuildConfiguration *>(bc);
    if (m_currentBc) {
        connect(m_currentBc, SIGNAL(busyConfigurationChanged()), this, SLOT(delayParsing()));
        delayParsing();
    } else {
        invalidate();
    }
}

void BusyProject::startParsing()
{
    // Busy does update the build graph during the build. So we cannot
    // start to parse while a build is running or we will lose information.
    if (BuildManager::isBuilding(this)) {
        scheduleParsing();
        return;
    }

    parseCurrentBuildConfiguration();
}

void BusyProject::delayParsing()
{
    m_parsingDelay.start();
}

void BusyProject::parseCurrentBuildConfiguration()
{
    m_parsingScheduled = false;
    if (m_cancelStatus == CancelStatusCancelingForReparse)
        return;

    // The CancelStatusCancelingAltoghether type can only be set by a build job, during
    // which no other parse requests come through to this point (except by the build job itself,
    // but of course not while canceling is in progress).
    QTC_ASSERT(m_cancelStatus == CancelStatusNone, return);

    if (!activeTarget())
        return;
    BusyBuildConfiguration *bc = qobject_cast<BusyBuildConfiguration *>(activeTarget()->activeBuildConfiguration());
    if (!bc)
        return;

    // New parse requests override old ones.
    // NOTE: We need to wait for the current operation to finish, since otherwise there could
    //       be a conflict. Consider the case where the old qbs::ProjectSetupJob is writing
    //       to the build graph file when the cancel request comes in. If we don't wait for
    //       acknowledgment, it might still be doing that when the new one already reads from the
    //       same file.
    if (m_qbsProjectParser) {
        m_cancelStatus = CancelStatusCancelingForReparse;
        m_qbsProjectParser->cancel();
        return;
    }

    parse(bc->busyConfiguration(), bc->environment(), bc->buildDirectory().toString());
}

void BusyProject::cancelParsing()
{
    QTC_ASSERT(m_qbsProjectParser, return);
    m_cancelStatus = CancelStatusCancelingAltoghether;
    m_qbsProjectParser->cancel();
}

void BusyProject::updateAfterBuild()
{
    QTC_ASSERT(m_qbsProject.isValid(), return);
    m_projectData = m_qbsProject.projectData();
    updateBuildTargetData();
    updateCppCompilerCallData();
}

void BusyProject::registerBusyProjectParser(BusyProjectParser *p)
{
    m_parsingDelay.stop();

    if (m_qbsProjectParser) {
        m_qbsProjectParser->disconnect(this);
        m_qbsProjectParser->deleteLater();
    }

    m_qbsProjectParser = p;

    if (p)
        connect(m_qbsProjectParser, SIGNAL(done(bool)), this, SLOT(handleBusyParsingDone(bool)));
}

Project::RestoreResult BusyProject::fromMap(const QVariantMap &map, QString *errorMessage)
{
    RestoreResult result = Project::fromMap(map, errorMessage);
    if (result != RestoreResult::Ok)
        return result;

    Kit *defaultKit = KitManager::defaultKit();
    if (!activeTarget() && defaultKit) {
        Target *t = new Target(this, defaultKit);
        t->updateDefaultBuildConfigurations();
        t->updateDefaultDeployConfigurations();
        t->updateDefaultRunConfigurations();
        addTarget(t);
    }

    return RestoreResult::Ok;
}

void BusyProject::generateErrors(const busy::ErrorInfo &e)
{
    foreach (const busy::ErrorItem &item, e.items())
        TaskHub::addTask(Task::Error, item.description(),
                         ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM,
                         FileName::fromString(item.codeLocation().filePath()),
                         item.codeLocation().line());

}

QString BusyProject::productDisplayName(const busy::Project &project,
                                       const busy::ProductData &product)
{
    QString displayName = product.name();
    if (product.profile() != project.profile())
        displayName.append(QLatin1String(" [")).append(product.profile()).append(QLatin1Char(']'));
    return displayName;
}

QString BusyProject::uniqueProductName(const busy::ProductData &product)
{
    return product.name() + QLatin1Char('.') + product.profile();
}

void BusyProject::parse(const QVariantMap &config, const Environment &env, const QString &dir)
{
    prepareForParsing();
    QTC_ASSERT(!m_qbsProjectParser, return);

    registerBusyProjectParser(new BusyProjectParser(this, m_qbsUpdateFutureInterface));

    BusyManager::instance()->updateProfileIfNecessary(activeTarget()->kit());
    m_qbsProjectParser->parse(config, env, dir);
    emit projectParsingStarted();
}

void BusyProject::prepareForParsing()
{
    TaskHub::clearTasks(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);
    if (m_qbsUpdateFutureInterface) {
        m_qbsUpdateFutureInterface->reportCanceled();
        m_qbsUpdateFutureInterface->reportFinished();
    }
    delete m_qbsUpdateFutureInterface;
    m_qbsUpdateFutureInterface = 0;

    m_qbsUpdateFutureInterface = new QFutureInterface<bool>();
    m_qbsUpdateFutureInterface->setProgressRange(0, 0);
    ProgressManager::addTask(m_qbsUpdateFutureInterface->future(),
        tr("Reading Project \"%1\"").arg(displayName()), "Busy.BusyEvaluate");
    m_qbsUpdateFutureInterface->reportStarted();
}

void BusyProject::updateDocuments(const QSet<QString> &files)
{
    // Update documents:
    QSet<QString> newFiles = files;
    QTC_ASSERT(!newFiles.isEmpty(), newFiles << m_fileName);
    QSet<QString> oldFiles;
    foreach (IDocument *doc, m_qbsDocuments)
        oldFiles.insert(doc->filePath().toString());

    QSet<QString> filesToAdd = newFiles;
    filesToAdd.subtract(oldFiles);
    QSet<QString> filesToRemove = oldFiles;
    filesToRemove.subtract(newFiles);

    QSet<IDocument *> currentDocuments = m_qbsDocuments;
    foreach (IDocument *doc, currentDocuments) {
        if (filesToRemove.contains(doc->filePath().toString())) {
            m_qbsDocuments.remove(doc);
            delete doc;
        }
    }
    QSet<IDocument *> toAdd;
    foreach (const QString &f, filesToAdd)
        toAdd.insert(new BusyProjectFile(this, f));

    DocumentManager::addDocuments(toAdd.toList());
    m_qbsDocuments.unite(toAdd);
}

void BusyProject::updateCppCodeModel()
{
    if (!m_projectData.isValid())
        return;

#if 0
    QtSupport::BaseQtVersion *qtVersion =
            QtSupport::QtKitInformation::qtVersion(activeTarget()->kit());
#endif

    CppTools::CppModelManager *modelmanager = CppTools::CppModelManager::instance();
    CppTools::ProjectInfo pinfo(this);
    CppTools::ProjectPartBuilder ppBuilder(pinfo);

#if 0
    if (qtVersion) {
        if (qtVersion->qtVersion() < QtSupport::QtVersionNumber(5,0,0))
            ppBuilder.setQtVersion(CppTools::ProjectPart::Qt4);
        else
            ppBuilder.setQtVersion(CppTools::ProjectPart::Qt5);
    } else
#endif
    {
        ppBuilder.setQtVersion(CppTools::ProjectPart::NoQt);
    }

    QHash<QString, QString> uiFiles;
    foreach (const busy::ProductData &prd, m_projectData.allProducts()) {
        foreach (const busy::GroupData &grp, prd.groups()) {
            const busy::PropertyMap &props = grp.properties();

            ppBuilder.setCxxFlags(props.getModulePropertiesAsStringList(
                                      QLatin1String(CONFIG_CPP_MODULE),
                                      QLatin1String(CONFIG_CXXFLAGS)));
            ppBuilder.setCFlags(props.getModulePropertiesAsStringList(
                                    QLatin1String(CONFIG_CPP_MODULE),
                                    QLatin1String(CONFIG_CFLAGS)));

            QStringList list = props.getModulePropertiesAsStringList(
                        QLatin1String(CONFIG_CPP_MODULE),
                        QLatin1String(CONFIG_DEFINES));
            QByteArray grpDefines;
            foreach (const QString &def, list) {
                QByteArray data = def.toUtf8();
                int pos = data.indexOf('=');
                if (pos >= 0)
                    data[pos] = ' ';
                else
                    data.append(" 1"); // cpp.defines: [ "FOO" ] is considered to be "FOO=1"
                grpDefines += (QByteArray("#define ") + data + '\n');
            }
            ppBuilder.setDefines(grpDefines);

            list = props.getModulePropertiesAsStringList(QLatin1String(CONFIG_CPP_MODULE),
                                                         QLatin1String(CONFIG_INCLUDEPATHS));
            list.append(props.getModulePropertiesAsStringList(QLatin1String(CONFIG_CPP_MODULE),
                                                              QLatin1String(CONFIG_SYSTEM_INCLUDEPATHS)));
            CppTools::ProjectPart::HeaderPaths grpHeaderPaths;
            foreach (const QString &p, list)
                grpHeaderPaths += CppTools::ProjectPart::HeaderPath(
                            FileName::fromUserInput(p).toString(),
                            CppTools::ProjectPart::HeaderPath::IncludePath);

            list = props.getModulePropertiesAsStringList(QLatin1String(CONFIG_CPP_MODULE),
                                                         QLatin1String(CONFIG_FRAMEWORKPATHS));
            list.append(props.getModulePropertiesAsStringList(QLatin1String(CONFIG_CPP_MODULE),
                                                              QLatin1String(CONFIG_SYSTEM_FRAMEWORKPATHS)));
            foreach (const QString &p, list)
                grpHeaderPaths += CppTools::ProjectPart::HeaderPath(
                            FileName::fromUserInput(p).toString(),
                            CppTools::ProjectPart::HeaderPath::FrameworkPath);

            ppBuilder.setHeaderPaths(grpHeaderPaths);

            const QString pch = props.getModuleProperty(QLatin1String(CONFIG_CPP_MODULE),
                    QLatin1String(CONFIG_PRECOMPILEDHEADER)).toString();
            ppBuilder.setPreCompiledHeaders(QStringList() << pch);

            ppBuilder.setDisplayName(grp.name());
            ppBuilder.setProjectFile(QString::fromLatin1("%1:%2:%3")
                    .arg(grp.location().filePath())
                    .arg(grp.location().line())
                    .arg(grp.location().column()));

            foreach (const QString &file, grp.allFilePaths()) {
                if (file.endsWith(QLatin1String(".ui"))) {
                    QStringList generated = m_rootProjectNode->busyProject()
                            .generatedFiles(prd, file, QStringList(QLatin1String("hpp")));
                    if (generated.count() == 1)
                        uiFiles.insert(file, generated.at(0));
                }
            }

            const QList<Id> languages =
                    ppBuilder.createProjectPartsForFiles(grp.allFilePaths());
            foreach (Id language, languages)
                setProjectLanguage(language, true);
        }
    }

    pinfo.finish();

    // QtSupport::UiCodeModelManager::update(this, uiFiles);

    // Update the code model
    m_codeModelFuture.cancel();
    m_codeModelFuture = modelmanager->updateProjectInfo(pinfo);
    m_codeModelProjectInfo = modelmanager->projectInfo(this);
    QTC_CHECK(m_codeModelProjectInfo == pinfo);
}

void BusyProject::updateCppCompilerCallData()
{
    CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();
    QTC_ASSERT(m_codeModelProjectInfo == modelManager->projectInfo(this), return);

    CppTools::ProjectInfo::CompilerCallData data;
    foreach (const busy::ProductData &product, m_projectData.allProducts()) {
        if (!product.isEnabled())
            continue;

        foreach (const busy::GroupData &group, product.groups()) {
            if (!group.isEnabled())
                continue;

            foreach (const QString &file, group.allFilePaths()) {
                if (!CppTools::ProjectFile::isSource(CppTools::ProjectFile::classify(file)))
                    continue;

                busy::ErrorInfo errorInfo;
                const busy::RuleCommandList ruleCommands
                       = m_qbsProject.ruleCommands(product, file, QLatin1String("obj"), &errorInfo);
                if (errorInfo.hasError())
                    continue;

                QList<QStringList> calls;
                foreach (const busy::RuleCommand &ruleCommand, ruleCommands) {
                    if (ruleCommand.type() == busy::RuleCommand::ProcessCommandType)
                        calls << ruleCommand.arguments();
                }

                if (!calls.isEmpty())
                    data.insert(file, calls);
            }
        }
    }

    m_codeModelProjectInfo.setCompilerCallData(data);
    const QFuture<void> future = modelManager->updateProjectInfo(m_codeModelProjectInfo);
    QTC_CHECK(future.isFinished()); // No reparse of files expected
}

void BusyProject::updateQmlJsCodeModel()
{
#if 0
    QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();
    if (!modelManager)
        return;

    QmlJS::ModelManagerInterface::ProjectInfo projectInfo =
            modelManager->defaultProjectInfoForProject(this);
    foreach (const qbs::ProductData &product, m_projectData.allProducts()) {
        static const QString propertyName = QLatin1String("qmlImportPaths");
        foreach (const QString &path, product.properties().value(propertyName).toStringList()) {
            projectInfo.importPaths.maybeInsert(Utils::FileName::fromString(path),
                                                QmlJS::Dialect::Qml);
        }
    }

    setProjectLanguage(ProjectExplorer::Constants::LANG_QMLJS, !projectInfo.sourceFiles.isEmpty());
    modelManager->updateProjectInfo(projectInfo, this);
#endif
}

void BusyProject::updateApplicationTargets()
{
    BuildTargetInfoList applications;
    foreach (const busy::ProductData &productData, m_projectData.allProducts()) {
        if (!productData.isEnabled() || !productData.isRunnable())
            continue;
        const QString displayName = productDisplayName(m_qbsProject, productData);
        if (productData.targetArtifacts().isEmpty()) { // No build yet.
            applications.list << BuildTargetInfo(displayName,
                    FileName(),
                    FileName::fromString(productData.location().filePath()));
            continue;
        }
        foreach (const busy::TargetArtifact &ta, productData.targetArtifacts()) {
            QTC_ASSERT(ta.isValid(), continue);
            if (!ta.isExecutable())
                continue;
            applications.list << BuildTargetInfo(displayName,
                    FileName::fromString(ta.filePath()),
                    FileName::fromString(productData.location().filePath()));
        }
    }
    activeTarget()->setApplicationTargets(applications);
}

void BusyProject::updateDeploymentInfo()
{
    DeploymentData deploymentData;
    if (m_qbsProject.isValid()) {
        busy::InstallOptions installOptions;
        installOptions.setInstallRoot(QLatin1String("/"));
        foreach (const busy::InstallableFile &f, m_qbsProject
                     .installableFilesForProject(m_projectData, installOptions)) {
            deploymentData.addFile(f.sourceFilePath(), QFileInfo(f.targetFilePath()).path(),
                    f.isExecutable() ? DeployableFile::TypeExecutable : DeployableFile::TypeNormal);
        }
    }
    activeTarget()->setDeploymentData(deploymentData);
}

void BusyProject::updateBuildTargetData()
{
    updateApplicationTargets();
    updateDeploymentInfo();
    foreach (Target *t, targets())
        t->updateDefaultRunConfigurations();
}

} // namespace Internal
} // namespace BusyProjectManager
