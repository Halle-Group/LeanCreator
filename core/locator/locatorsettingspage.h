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

#ifndef LOCATORSETTINGSPAGE_H
#define LOCATORSETTINGSPAGE_H

#include "ui_locatorsettingspage.h"

#include <core/dialogs/ioptionspage.h>

#include <QHash>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QListWidgetItem;
class QSortFilterProxyModel;
QT_END_NAMESPACE

namespace Utils {

class TreeModel;
class TreeItem;

} // Utils

namespace Core {

class ILocatorFilter;

namespace Internal {

class Locator;

class LocatorSettingsPage : public IOptionsPage
{
    Q_OBJECT

public:
    explicit LocatorSettingsPage(Locator *plugin);

    QWidget *widget();
    void apply();
    void finish();

private slots:
    void updateButtonStates();
    void configureFilter(const QModelIndex &proxyIndex);
    void addCustomFilter();
    void removeCustomFilter();

private:
    void initializeModel();
    void saveFilterStates();
    void restoreFilterStates();
    void requestRefresh();
    void setFilter(const QString &text);

    Ui::LocatorSettingsWidget m_ui;
    Locator *m_plugin;
    QPointer<QWidget> m_widget;
    Utils::TreeModel *m_model;
    QSortFilterProxyModel *m_proxyModel;
    Utils::TreeItem *m_customFilterRoot;
    QList<ILocatorFilter *> m_filters;
    QList<ILocatorFilter *> m_addedFilters;
    QList<ILocatorFilter *> m_removedFilters;
    QList<ILocatorFilter *> m_customFilters;
    QList<ILocatorFilter *> m_refreshFilters;
    QHash<ILocatorFilter *, QByteArray> m_filterStates;
};

} // namespace Internal
} // namespace Core

#endif // LOCATORSETTINGSPAGE_H
