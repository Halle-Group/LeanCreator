/****************************************************************************
**
** Copyright (C) 2015 Petar Perisin <petar.perisin@gmail.com>
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

#ifndef UNSTARTEDAPPWATCHERDIALOG_H
#define UNSTARTEDAPPWATCHERDIALOG_H

#include <QDialog>
#include <QTimer>

#ifndef QT_NO_DEVICESUPPORT
#include <projectexplorer/devicesupport/deviceprocesslist.h>
#endif

QT_BEGIN_NAMESPACE
class QLabel;
class QCheckBox;
QT_END_NAMESPACE

namespace ProjectExplorer {
class KitChooser;
class Kit;
}

namespace Utils { class PathChooser; }

namespace Debugger {
namespace Internal {

class UnstartedAppWatcherDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UnstartedAppWatcherDialog(QWidget *parent = 0);

    ProjectExplorer::Kit *currentKit() const;
#ifndef QT_NO_DEVICESUPPORT
    ProjectExplorer::DeviceProcessItem currentProcess() const;
#endif
    bool hideOnAttach() const;
    bool continueOnAttach() const;
    void startWatching();

    bool event(QEvent *) override;

signals:
    void processFound();

private:
    void selectExecutable();
    void pidFound(
        #ifndef QT_NO_DEVICESUPPORT
            const ProjectExplorer::DeviceProcessItem &p
        #endif
            );
    void startStopWatching(bool start);
    void findProcess();
    void stopAndCheckExecutable();
    void kitChanged();

    enum UnstartedAppWacherState
    {
        InvalidWacherState,
        NotWatchingState,
        WatchingState,
        FoundState
    };

    void startStopTimer(bool start);
    bool checkExecutableString() const;
    void setWaitingState(UnstartedAppWacherState state);

    ProjectExplorer::KitChooser *m_kitChooser;
    Utils::PathChooser *m_pathChooser;
    QLabel *m_waitingLabel;
    QCheckBox *m_hideOnAttachCheckBox;
    QCheckBox *m_continueOnAttachCheckBox;
    QPushButton *m_watchingPushButton;
#ifndef QT_NO_DEVICESUPPORT
    ProjectExplorer::DeviceProcessItem m_process;
#endif
    QTimer m_timer;
};

} // namespace Internal
} // namespace Debugger

#endif // UNSTARTEDAPPWATCHERDIALOG_H
