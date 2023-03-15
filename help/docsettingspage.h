/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Copyright (C) 2023 Rochus Keller (me@rochus-keller.ch) for LeanCreator
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

#ifndef DOCSETTINGSPAGE_H
#define DOCSETTINGSPAGE_H

#include "ui_docsettingspage.h"
#include <core/dialogs/ioptionspage.h>

#include <QPointer>

namespace Help {
namespace Internal {

class DocSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    DocSettingsPage();

    QWidget *widget();
    void apply();
    void finish();

private slots:
    void addDocumentation();
    void removeDocumentation();

private:
    bool eventFilter(QObject *object, QEvent *event);
    void removeDocumentation(const QList<QListWidgetItem *> &items);
    void addItem(const QString &nameSpace, const QString &fileName, bool userManaged);

private:
    Ui::DocSettingsPage m_ui;
    QPointer<QWidget> m_widget;

    QString m_recentDialogPath;

    typedef QHash<QString, QString> NameSpaceToPathHash;
    NameSpaceToPathHash m_filesToRegister;
    QHash<QString, bool> m_filesToRegisterUserManaged;
    NameSpaceToPathHash m_filesToUnregister;
};

} // namespace Help
} // namespace Internal

#endif // DOCSETTINGSPAGE_H
