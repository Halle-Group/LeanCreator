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

#ifndef PROJECTEXPLORERSETTINGSPAGE_H
#define PROJECTEXPLORERSETTINGSPAGE_H

#include <core/dialogs/ioptionspage.h>
#include "ui_projectexplorersettingspage.h"

#include <QPointer>
#include <QUuid>

namespace ProjectExplorer {
namespace Internal {

class ProjectExplorerSettings;

// Documentation inside.
class ProjectExplorerSettingsWidget : public QWidget {
    Q_OBJECT
public:
    explicit ProjectExplorerSettingsWidget(QWidget *parent = 0);

    ProjectExplorerSettings settings() const;
    void setSettings(const ProjectExplorerSettings  &s);

    QString projectsDirectory() const;
    void setProjectsDirectory(const QString &pd);

    bool useProjectsDirectory();
    void setUseProjectsDirectory(bool v);

    QString buildDirectory() const;
    void setBuildDirectory(const QString &bd);

private slots:
    void slotDirectoryButtonGroupChanged();
    void resetDefaultBuildDirectory();
    void updateResetButton();

private:
    void setJomVisible(bool);

    Ui::ProjectExplorerSettingsPageUi m_ui;
    QUuid m_environmentId;
};

class ProjectExplorerSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    ProjectExplorerSettingsPage();
    ~ProjectExplorerSettingsPage();

    QWidget *widget();
    void apply();
    void finish();

private:
    QPointer<ProjectExplorerSettingsWidget> m_widget;
};

} // Internal
} // ProjectExplorer

#endif // PROJECTEXPLORERSETTINGSPAGE_H
