/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Copyright (C) 2022 Rochus Keller (me@rochus-keller.ch) for LeanCreator
**
** This file is part of Qt Creator.
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

#include "genericprojectplugin.h"

#include "genericbuildconfiguration.h"
#include "genericprojectmanager.h"
#include "genericprojectwizard.h"
#include "genericprojectconstants.h"
#include "genericprojectfileseditor.h"
#include "genericmakestep.h"
#include "genericproject.h"

#include <core/icore.h>
#include <core/actionmanager/actionmanager.h>
#include <core/actionmanager/actioncontainer.h>
#include <core/actionmanager/command.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/selectablefilesmodel.h>

#include <utils/mimetypes/mimedatabase.h>

#include <QtPlugin>
#include <QDebug>

using namespace Core;
using namespace ProjectExplorer;

namespace GenericProjectManager {
namespace Internal {

bool GenericProjectPlugin::initialize(const QStringList &, QString *errorMessage)
{
    Q_UNUSED(errorMessage)
    Utils::MimeDatabase::addMimeTypes(QLatin1String(":genericproject/GenericProjectManager.mimetypes.xml"));

    addAutoReleasedObject(new Manager);
    addAutoReleasedObject(new ProjectFilesFactory);
    addAutoReleasedObject(new GenericMakeStepFactory);
    addAutoReleasedObject(new GenericBuildConfigurationFactory);

    IWizardFactory::registerFactoryCreator([]() { return QList<IWizardFactory *>() << new GenericProjectWizard; });

    ActionContainer *mproject =
            ActionManager::actionContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT);

    auto editFilesAction = new QAction(tr("Edit Files..."), this);
    Command *command = ActionManager::registerAction(editFilesAction,
        "GenericProjectManager.EditFiles", Context(Constants::PROJECTCONTEXT));
    command->setAttribute(Command::CA_Hide);
    mproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_FILES);

    connect(editFilesAction, &QAction::triggered, this, &GenericProjectPlugin::editFiles);

    return true;
}

void GenericProjectPlugin::editFiles()
{
    auto genericProject = qobject_cast<GenericProject *>(ProjectTree::currentProject());
    if (!genericProject)
        return;
    SelectableFilesDialogEditFiles sfd(genericProject->projectFilePath().toFileInfo().path(), genericProject->files(),
                              ICore::mainWindow());
    if (sfd.exec() == QDialog::Accepted)
        genericProject->setFiles(sfd.selectedFiles());
}

} // namespace Internal
} // namespace GenericProjectManager
