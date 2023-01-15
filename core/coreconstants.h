/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of LeanCreator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CORECONSTANTS_H
#define CORECONSTANTS_H

#include <QtGlobal>

namespace Core {
namespace Constants {

// Modes
const char MODE_WELCOME[]          = "Welcome";
const char MODE_EDIT[]             = "Edit";
const char MODE_DESIGN[]           = "Design";
const int  P_MODE_WELCOME          = 100;
const int  P_MODE_EDIT             = 90;
const int  P_MODE_DESIGN           = 89;

// Menubar
const char MENU_BAR[]              = "LeanCreator.MenuBar";

// Menus
const char M_FILE[]                = "LeanCreator.Menu.File";
const char M_FILE_RECENTFILES[]    = "LeanCreator.Menu.File.RecentFiles";
const char M_EDIT[]                = "LeanCreator.Menu.Edit";
const char M_EDIT_ADVANCED[]       = "LeanCreator.Menu.Edit.Advanced";
const char M_TOOLS[]               = "LeanCreator.Menu.Tools";
const char M_TOOLS_EXTERNAL[]      = "LeanCreator.Menu.Tools.External";
const char M_WINDOW[]              = "LeanCreator.Menu.Window";
const char M_WINDOW_PANES[]        = "LeanCreator.Menu.Window.Panes";
const char M_WINDOW_VIEWS[]        = "LeanCreator.Menu.Window.Views";
const char M_HELP[]                = "LeanCreator.Menu.Help";

// Contexts
const char C_GLOBAL[]              = "Global Context";
const char C_WELCOME_MODE[]        = "Core.WelcomeMode";
const char C_EDIT_MODE[]           = "Core.EditMode";
const char C_DESIGN_MODE[]         = "Core.DesignMode";
const char C_EDITORMANAGER[]       = "Core.EditorManager";
const char C_NAVIGATION_PANE[]     = "Core.NavigationPane";
const char C_PROBLEM_PANE[]        = "Core.ProblemPane";
const char C_GENERAL_OUTPUT_PANE[] = "Core.GeneralOutputPane";

// Default editor kind
const char K_DEFAULT_TEXT_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("OpenWith::Editors", "Plain Text Editor");
const char K_DEFAULT_TEXT_EDITOR_ID[] = "Core.PlainTextEditor";
const char K_DEFAULT_BINARY_EDITOR_ID[] = "Core.BinaryEditor";

//actions
const char UNDO[]                  = "LeanCreator.Undo";
const char REDO[]                  = "LeanCreator.Redo";
const char COPY[]                  = "LeanCreator.Copy";
const char PASTE[]                 = "LeanCreator.Paste";
const char CUT[]                   = "LeanCreator.Cut";
const char SELECTALL[]             = "LeanCreator.SelectAll";

const char GOTO[]                  = "LeanCreator.Goto";

const char NEW[]                   = "LeanCreator.New";
const char OPEN[]                  = "LeanCreator.Open";
const char OPEN_WITH[]             = "LeanCreator.OpenWith";
const char REVERTTOSAVED[]         = "LeanCreator.RevertToSaved";
const char SAVE[]                  = "LeanCreator.Save";
const char SAVEAS[]                = "LeanCreator.SaveAs";
const char SAVEALL[]               = "LeanCreator.SaveAll";
const char PRINT[]                 = "LeanCreator.Print";
const char EXIT[]                  = "LeanCreator.Exit";

const char OPTIONS[]               = "LeanCreator.Options";
const char TOGGLE_SIDEBAR[]        = "LeanCreator.ToggleSidebar";
const char TOGGLE_MODE_SELECTOR[]  = "LeanCreator.ToggleModeSelector";
const char TOGGLE_FULLSCREEN[]     = "LeanCreator.ToggleFullScreen";
const char THEMEOPTIONS[]          = "LeanCreator.ThemeOptions";

const char TR_SHOW_SIDEBAR[]       = QT_TRANSLATE_NOOP("Core", "Show Sidebar");
const char TR_HIDE_SIDEBAR[]       = QT_TRANSLATE_NOOP("Core", "Hide Sidebar");

const char MINIMIZE_WINDOW[]       = "LeanCreator.MinimizeWindow";
const char ZOOM_WINDOW[]           = "LeanCreator.ZoomWindow";
const char CLOSE_WINDOW[]           = "LeanCreator.CloseWindow";

const char SPLIT[]                 = "LeanCreator.Split";
const char SPLIT_SIDE_BY_SIDE[]    = "LeanCreator.SplitSideBySide";
const char SPLIT_NEW_WINDOW[]      = "LeanCreator.SplitNewWindow";
const char REMOVE_CURRENT_SPLIT[]  = "LeanCreator.RemoveCurrentSplit";
const char REMOVE_ALL_SPLITS[]     = "LeanCreator.RemoveAllSplits";
const char GOTO_NEXT_SPLIT[]      = "LeanCreator.GotoOtherSplit";
const char CLOSE[]                 = "LeanCreator.Close";
const char CLOSE_ALTERNATIVE[]     = "LeanCreator.Close_Alternative"; // temporary, see QTCREATORBUG-72
const char CLOSEALL[]              = "LeanCreator.CloseAll";
const char CLOSEOTHERS[]           = "LeanCreator.CloseOthers";
const char CLOSEALLEXCEPTVISIBLE[] = "LeanCreator.CloseAllExceptVisible";
const char GOTONEXT[]              = "LeanCreator.GotoNext";
const char GOTOPREV[]              = "LeanCreator.GotoPrevious";
const char GOTONEXTINHISTORY[]     = "LeanCreator.GotoNextInHistory";
const char GOTOPREVINHISTORY[]     = "LeanCreator.GotoPreviousInHistory";
const char GO_BACK[]               = "LeanCreator.GoBack";
const char GO_FORWARD[]            = "LeanCreator.GoForward";
const char ABOUT_QTCREATOR[]       = "LeanCreator.AboutQtCreator";
const char ABOUT_QT[]              = "LeanCreator.AboutQt";
const char ABOUT_PLUGINS[]         = "LeanCreator.AboutPlugins";
const char S_RETURNTOEDITOR[]      = "LeanCreator.ReturnToEditor";

// Default groups
const char G_DEFAULT_ONE[]         = "LeanCreator.Group.Default.One";
const char G_DEFAULT_TWO[]         = "LeanCreator.Group.Default.Two";
const char G_DEFAULT_THREE[]       = "LeanCreator.Group.Default.Three";

// Main menu bar groups
const char G_FILE[]                = "LeanCreator.Group.File";
const char G_EDIT[]                = "LeanCreator.Group.Edit";
const char G_VIEW[]                = "LeanCreator.Group.View";
const char G_TOOLS[]               = "LeanCreator.Group.Tools";
const char G_WINDOW[]              = "LeanCreator.Group.Window";
const char G_HELP[]                = "LeanCreator.Group.Help";

// File menu groups
const char G_FILE_NEW[]            = "LeanCreator.Group.File.New";
const char G_FILE_OPEN[]           = "LeanCreator.Group.File.Open";
const char G_FILE_PROJECT[]        = "LeanCreator.Group.File.Project";
const char G_FILE_SAVE[]           = "LeanCreator.Group.File.Save";
const char G_FILE_CLOSE[]          = "LeanCreator.Group.File.Close";
const char G_FILE_PRINT[]          = "LeanCreator.Group.File.Print";
const char G_FILE_OTHER[]          = "LeanCreator.Group.File.Other";

// Edit menu groups
const char G_EDIT_UNDOREDO[]       = "LeanCreator.Group.Edit.UndoRedo";
const char G_EDIT_COPYPASTE[]      = "LeanCreator.Group.Edit.CopyPaste";
const char G_EDIT_SELECTALL[]      = "LeanCreator.Group.Edit.SelectAll";
const char G_EDIT_ADVANCED[]       = "LeanCreator.Group.Edit.Advanced";

const char G_EDIT_FIND[]           = "LeanCreator.Group.Edit.Find";
const char G_EDIT_OTHER[]          = "LeanCreator.Group.Edit.Other";

// Advanced edit menu groups
const char G_EDIT_FORMAT[]         = "LeanCreator.Group.Edit.Format";
const char G_EDIT_COLLAPSING[]     = "LeanCreator.Group.Edit.Collapsing";
const char G_EDIT_TEXT[]           = "LeanCreator.Group.Edit.Text";
const char G_EDIT_BLOCKS[]         = "LeanCreator.Group.Edit.Blocks";
const char G_EDIT_FONT[]           = "LeanCreator.Group.Edit.Font";
const char G_EDIT_EDITOR[]         = "LeanCreator.Group.Edit.Editor";

const char G_TOOLS_OPTIONS[]       = "LeanCreator.Group.Tools.Options";

// Window menu groups
const char G_WINDOW_SIZE[]         = "LeanCreator.Group.Window.Size";
const char G_WINDOW_PANES[]        = "LeanCreator.Group.Window.Panes";
const char G_WINDOW_VIEWS[]        = "LeanCreator.Group.Window.Views";
const char G_WINDOW_SPLIT[]        = "LeanCreator.Group.Window.Split";
const char G_WINDOW_NAVIGATE[]     = "LeanCreator.Group.Window.Navigate";
const char G_WINDOW_LIST[]         = "LeanCreator.Group.Window.List";
const char G_WINDOW_OTHER[]        = "LeanCreator.Group.Window.Other";

// Help groups (global)
const char G_HELP_HELP[]           = "LeanCreator.Group.Help.Help";
const char G_HELP_SUPPORT[]        = "LeanCreator.Group.Help.Supprt";
const char G_HELP_ABOUT[]          = "LeanCreator.Group.Help.About";
const char G_HELP_UPDATES[]        = "LeanCreator.Group.Help.Updates";

const char ICON_MINUS[]              = ":/core/images/minus.png";
const char ICON_PLUS[]               = ":/core/images/plus.png";
const char ICON_NEWFILE[]            = ":/core/images/filenew.png";
const char ICON_OPENFILE[]           = ":/core/images/fileopen.png";
const char ICON_SAVEFILE[]           = ":/core/images/filesave.png";
const char ICON_UNDO[]               = ":/core/images/undo.png";
const char ICON_REDO[]               = ":/core/images/redo.png";
const char ICON_COPY[]               = ":/core/images/editcopy.png";
const char ICON_PASTE[]              = ":/core/images/editpaste.png";
const char ICON_CUT[]                = ":/core/images/editcut.png";
const char ICON_NEXT[]               = ":/core/images/next.png";
const char ICON_PREV[]               = ":/core/images/prev.png";
const char ICON_DIR[]                = ":/core/images/dir.png";
const char ICON_CLEAN_PANE[]         = ":/core/images/clean_pane_small.png";
const char ICON_CLEAR[]              = ":/core/images/clear.png";
const char ICON_RESET[]              = ":/core/images/reset.png";
const char ICON_RELOAD_GRAY[]        = ":/core/images/reload_gray.png";
const char ICON_MAGNIFIER[]          = ":/core/images/magnifier.png";
const char ICON_TOGGLE_SIDEBAR[]     = ":/core/images/sidebaricon.png";
const char ICON_BUTTON_CLOSE[]       = ":/core/images/button_close.png";
const char ICON_CLOSE_BUTTON[]       = ":/core/images/closebutton.png";
const char ICON_DARK_CLOSE_BUTTON[]  = ":/core/images/darkclosebutton.png";
const char ICON_DARK_CLOSE[]         = ":/core/images/darkclose.png";
const char ICON_SPLIT_HORIZONTAL[]   = ":/core/images/splitbutton_horizontal.png";
const char ICON_SPLIT_VERTICAL[]     = ":/core/images/splitbutton_vertical.png";
const char ICON_CLOSE_SPLIT_TOP[]    = ":/core/images/splitbutton_closetop.png";
const char ICON_CLOSE_SPLIT_BOTTOM[] = ":/core/images/splitbutton_closebottom.png";
const char ICON_CLOSE_SPLIT_LEFT[]   = ":/core/images/splitbutton_closeleft.png";
const char ICON_CLOSE_SPLIT_RIGHT[]  = ":/core/images/splitbutton_closeright.png";
const char ICON_FILTER[]             = ":/core/images/filtericon.png";
const char ICON_LINK[]               = ":/core/images/linkicon.png";
const char ICON_PAUSE[]              = ":/core/images/pause.png";
const char ICON_QTLOGO_32[]          = ":/core/images/logo/32/QtProject-qtcreator.png";
const char ICON_QTLOGO_64[]          = ":/core/images/logo/64/QtProject-qtcreator.png";
const char ICON_QTLOGO_128[]         = ":/core/images/logo/128/QtProject-qtcreator.png";
const char ICON_WARNING[]            = ":/core/images/warning.png";
const char ICON_ERROR[]              = ":/core/images/error.png";
const char ICON_INFO[]               = ":/core/images/info.png";

const char WIZARD_CATEGORY_QT[] = "R.Qt";
const char WIZARD_TR_CATEGORY_QT[] = QT_TRANSLATE_NOOP("Core", "Qt");
const char WIZARD_KIND_UNKNOWN[] = "unknown";
const char WIZARD_KIND_PROJECT[] = "project";
const char WIZARD_KIND_FILE[] = "file";

const char SETTINGS_CATEGORY_CORE[] = "A.Core";
const char SETTINGS_CATEGORY_CORE_ICON[] = ":/core/images/category_core.png";
const char SETTINGS_TR_CATEGORY_CORE[] = QT_TRANSLATE_NOOP("Core", "Environment");
const char SETTINGS_ID_INTERFACE[] = "A.Interface";
const char SETTINGS_ID_SYSTEM[] = "B.Core.System";
const char SETTINGS_ID_SHORTCUTS[] = "C.Keyboard";
const char SETTINGS_ID_TOOLS[] = "D.ExternalTools";
const char SETTINGS_ID_MIMETYPES[] = "E.MimeTypes";

const char SETTINGS_DEFAULTTEXTENCODING[] = "General/DefaultFileEncoding";

const char SETTINGS_THEME[] = "Core/CreatorTheme";

const char ALL_FILES_FILTER[]      = QT_TRANSLATE_NOOP("Core", "All Files (*)");

const char TR_CLEAR_MENU[]         = QT_TRANSLATE_NOOP("Core", "Clear Menu");

const char DEFAULT_BUILD_DIRECTORY[] = "../build-%{CurrentProject:Name}-%{CurrentKit:FileSystemName}-%{CurrentBuild:Name}";

const int TARGET_ICON_SIZE = 32;

} // namespace Constants
} // namespace Core

#endif // CORECONSTANTS_H
