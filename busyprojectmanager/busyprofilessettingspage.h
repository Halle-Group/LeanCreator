/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef BUSYPROFILESSETTINGSPAGE_H
#define BUSYPROFILESSETTINGSPAGE_H

#include <core/dialogs/ioptionspage.h>

namespace BusyProjectManager {
namespace Internal {
class BusyProfilesSettingsWidget;

class BusyProfilesSettingsPage : public Core::IOptionsPage
{
public:
    BusyProfilesSettingsPage(QObject *parent = 0);

private:
    QWidget *widget() override;
    void apply() override;
    void finish() override;

    BusyProfilesSettingsWidget *m_widget;
};

} // namespace Internal
} // namespace BusyProjectManager

#endif // Include guard.
