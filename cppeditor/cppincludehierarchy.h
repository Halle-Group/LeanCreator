/****************************************************************************
**
** Copyright (C) 2015 Przemyslaw Gorszkowski <pgorszkowski@gmail.com>
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

#ifndef CPPINCLUDEHIERARCHY_H
#define CPPINCLUDEHIERARCHY_H

#include <core/inavigationwidgetfactory.h>

#include <QString>
#include <QStackedWidget>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QStandardItemModel;
class QStandardItem;
class QModelIndex;
class QLabel;
QT_END_NAMESPACE

namespace Core { class IEditor; }

namespace TextEditor {
class BaseTextEditor;
class TextEditorLinkLabel;
}

namespace Utils {
class AnnotatedItemDelegate;
class FileName;
}

namespace CppEditor {
namespace Internal {

class CppEditor;
class CppEditorWidget;
class CppInclude;
class CppIncludeHierarchyModel;
class CppIncludeHierarchyTreeView;

class CppIncludeHierarchyWidget : public QWidget
{
    Q_OBJECT
public:
    CppIncludeHierarchyWidget();
    virtual ~CppIncludeHierarchyWidget();

public slots:
    void perform();

private slots:
    void onItemActivated(const QModelIndex &index);
    void editorsClosed(QList<Core::IEditor *> editors);

private:
    void showNoIncludeHierarchyLabel();
    void showIncludeHierarchy();

    CppEditorWidget *m_cppEditor;
    CppIncludeHierarchyTreeView *m_treeView;
    CppIncludeHierarchyModel *m_model;
    Utils::AnnotatedItemDelegate *m_delegate;
    TextEditor::TextEditorLinkLabel *m_inspectedFile;
    QLabel *m_includeHierarchyInfoLabel;
    TextEditor::BaseTextEditor *m_editor;
};

// @todo: Pretty much the same design as the OutlineWidgetStack. Maybe we can generalize the
// outline factory so that it works for different widgets that support the same editor.
class CppIncludeHierarchyStackedWidget : public QStackedWidget
{
    Q_OBJECT
public:
    CppIncludeHierarchyStackedWidget(QWidget *parent = 0);
    virtual ~CppIncludeHierarchyStackedWidget();

private:
    CppIncludeHierarchyWidget *m_typeHiearchyWidgetInstance;
};

class CppIncludeHierarchyFactory : public Core::INavigationWidgetFactory
{
    Q_OBJECT
public:
    CppIncludeHierarchyFactory();

    Core::NavigationView createWidget();
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPINCLUDEHIERARCHY_H
