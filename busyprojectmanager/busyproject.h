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

#ifndef BUSYPROJECT_H
#define BUSYPROJECT_H

#include "busyprojectmanager.h"

#include <cpptools/cppprojects.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/task.h>

#include <utils/environment.h>

#include <busy/busyapi.h>

#include <QFuture>
#include <QTimer>

namespace Core { class IDocument; }
namespace ProjectExplorer { class BuildConfiguration; }

namespace BusyProjectManager {
namespace Internal {
class BusyBaseProjectNode;
class BusyProjectNode;
class BusyRootProjectNode;
class BusyProjectParser;
class BusyBuildConfiguration;

class BusyProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    BusyProject(BusyManager *manager, const QString &filename);
    ~BusyProject();

    QString displayName() const;
    Core::IDocument *document() const;
    BusyManager *projectManager() const;

    ProjectExplorer::ProjectNode *rootProjectNode() const;

    QStringList files(FilesMode fileMode) const;

    bool isProjectEditable() const;
    bool addFilesToProduct(BusyBaseProjectNode *node, const QStringList &filePaths,
                           const busy::ProductData &productData, const busy::GroupData &groupData,
                           QStringList *notAdded);
    bool removeFilesFromProduct(BusyBaseProjectNode *node, const QStringList &filePaths,
            const busy::ProductData &productData, const busy::GroupData &groupData,
            QStringList *notRemoved);
    bool renameFileInProduct(BusyBaseProjectNode *node, const QString &oldPath,
            const QString &newPath, const busy::ProductData &productData,
            const busy::GroupData &groupData);

    busy::BuildJob *build(const busy::BuildOptions &opts, QStringList products, QString &error);
    busy::CleanJob *clean(const busy::CleanOptions &opts);
    busy::InstallJob *install(const busy::InstallOptions &opts);

    static ProjectExplorer::FileType fileTypeFor(const QSet<QString> &tags);

    QString profileForTarget(const ProjectExplorer::Target *t) const;
    bool isParsing() const;
    bool hasParseResult() const;
    void parseCurrentBuildConfiguration();
    void scheduleParsing() { m_parsingScheduled = true; }
    bool parsingScheduled() const { return m_parsingScheduled; }
    void cancelParsing();
    void updateAfterBuild();

    void registerBusyProjectParser(BusyProjectParser *p);

    busy::Project busyProject() const;
    busy::ProjectData busyProjectData() const;

    bool needsSpecialDeployment() const;
    void generateErrors(const busy::ErrorInfo &e);

    static QString productDisplayName(const busy::Project &project,
                                      const busy::ProductData &product);
    static QString uniqueProductName(const busy::ProductData &product);

public slots:
    void invalidate();
    void delayParsing();

signals:
    void projectParsingStarted();
    void projectParsingDone(bool);

private slots:
    void handleBusyParsingDone(bool success);

    void targetWasAdded(ProjectExplorer::Target *t);
    void changeActiveTarget(ProjectExplorer::Target *t);
    void buildConfigurationChanged(ProjectExplorer::BuildConfiguration *bc);
    void startParsing();

private:
    RestoreResult fromMap(const QVariantMap &map, QString *errorMessage);

    void parse(const QVariantMap &config, const Utils::Environment &env, const QString &dir);

    void prepareForParsing();
    void updateDocuments(const QSet<QString> &files);
    void updateCppCodeModel();
    void updateCppCompilerCallData();
    void updateQmlJsCodeModel();
    void updateApplicationTargets();
    void updateDeploymentInfo();
    void updateBuildTargetData();

    static bool ensureWriteableBusyFile(const QString &file);

    busy::GroupData reRetrieveGroupData(const busy::ProductData &oldProduct,
                                       const busy::GroupData &oldGroup);

    BusyManager *const m_manager;
    const QString m_projectName;
    const QString m_fileName;
    busy::Project m_qbsProject;
    busy::ProjectData m_projectData;
    QSet<Core::IDocument *> m_qbsDocuments;
    BusyRootProjectNode *m_rootProjectNode;

    BusyProjectParser *m_qbsProjectParser;

    QFutureInterface<bool> *m_qbsUpdateFutureInterface;
    bool m_parsingScheduled;

    enum CancelStatus {
        CancelStatusNone,
        CancelStatusCancelingForReparse,
        CancelStatusCancelingAltoghether
    } m_cancelStatus;

    QFuture<void> m_codeModelFuture;
    CppTools::ProjectInfo m_codeModelProjectInfo;

    BusyBuildConfiguration *m_currentBc;

    QTimer m_parsingDelay;
};

} // namespace Internal
} // namespace BusyProjectManager

#endif // BUSYPROJECT_H
