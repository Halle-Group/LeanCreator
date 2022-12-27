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

#include "copytaskhandler.h"

#include "task.h"

#include <core/coreconstants.h>

#include <QAction>
#include <QApplication>
#include <QClipboard>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

void CopyTaskHandler::handle(const Task &task)
{
    QString type;
    switch (task.type) {
    case Task::Error:
        //: Task is of type: error
        type = tr("error:") + QLatin1Char(' ');
        break;
    case Task::Warning:
        //: Task is of type: warning
        type = tr("warning:") + QLatin1Char(' ');
        break;
    default:
        break;
    }

    QApplication::clipboard()->setText(task.file.toUserOutput() + QLatin1Char(':') +
                                       QString::number(task.line) + QLatin1String(": ")
                                       + type + task.description);
}

Core::Id CopyTaskHandler::actionManagerId() const
{
    return Core::Id(Core::Constants::COPY);
}

QAction *CopyTaskHandler::createAction(QObject *parent) const
{
    QAction *copyAction = new QAction(parent);
    return copyAction;
}
