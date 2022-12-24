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

#include "task.h"

#include <core/coreconstants.h>
#include <utils/qtcassert.h>

#include "projectexplorerconstants.h"

namespace ProjectExplorer
{

static QIcon taskTypeIcon(Task::TaskType t)
{
    static QIcon icons[3] = { QIcon(),
                              QIcon(QLatin1String(Core::Constants::ICON_ERROR)),
                              QIcon(QLatin1String(Core::Constants::ICON_WARNING)) };

    if (t < 0 || t > 2)
        t = Task::Unknown;

    return icons[t];
}

unsigned int Task::s_nextId = 1;

/*!
    \class  ProjectExplorer::Task
    \brief The Task class represents a build issue (warning or error).
    \sa ProjectExplorer::TaskHub
*/

Task::Task() : taskId(0), type(Unknown), line(-1), movedLine(-1)
{ }

Task::Task(TaskType type_, const QString &description_,
           const Utils::FileName &file_, int line_, Core::Id category_,
           const Utils::FileName &iconFile) :
    taskId(s_nextId), type(type_), description(description_),
    file(file_), line(line_), movedLine(line_), category(category_),
    icon(iconFile.isEmpty() ? taskTypeIcon(type_) : QIcon(iconFile.toString()))
{
    ++s_nextId;
}

Task Task::compilerMissingTask()
{
    return Task(Task::Error,
                QCoreApplication::translate("ProjectExplorer::Task",
                                            "Qt Creator needs a compiler set up to build. "
                                            "Configure a compiler in the kit options."),
                Utils::FileName(), -1,
                Constants::TASK_CATEGORY_BUILDSYSTEM);
}

Task Task::buildConfigurationMissingTask()
{
    return Task(Task::Error,
                QCoreApplication::translate("ProjectExplorer::Task",
                                            "Qt Creator needs a build configuration set up to build. "
                                            "Configure a build configuration in the project settings."),
                Utils::FileName(), -1,
                Constants::TASK_CATEGORY_BUILDSYSTEM);
}

void Task::addMark(TextEditor::TextMark *mark)
{
    QTC_ASSERT(m_mark.isNull(), return);

    m_mark = QSharedPointer<TextEditor::TextMark>(mark);
}

bool Task::isNull() const
{
    return taskId == 0;
}

void Task::clear()
{
    taskId = 0;
    description.clear();
    file = Utils::FileName();
    line = -1;
    movedLine = -1;
    category = Core::Id();
    type = Task::Unknown;
    icon = QIcon();
}

//
// functions
//
bool operator==(const Task &t1, const Task &t2)
{
    return t1.taskId == t2.taskId;
}

bool operator<(const Task &a, const Task &b)
{
    if (a.type != b.type) {
        if (a.type == Task::Error)
            return true;
        if (b.type == Task::Error)
            return false;
        if (a.type == Task::Warning)
            return true;
        if (b.type == Task::Warning)
            return false;
        // Can't happen
        return true;
    } else {
        if (a.category < b.category)
            return true;
        if (b.category < a.category)
            return false;
        return a.taskId < b.taskId;
    }
}


uint qHash(const Task &task)
{
    return task.taskId;
}

} // namespace ProjectExplorer
