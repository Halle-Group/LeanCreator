![icon](http://software.rochus-keller.ch/creator-logo-100x460.png)

## Welcome to LeanCreator - a lean version of Qt Creator

LeanCreator is a stripped-down version of Qt Creator, which includes the essential features for C and C++ development, and is easy to build from source on all platforms supported by LeanQt.

LeanCreator uses [LeanQt](https://github.com/rochus-keller/LeanQt) instead of the original Qt toolkit, and is built with and uses the [BUSY build system](https://github.com/rochus-keller/BUSY) instead of qmake.

In contrast to QtCreator, LeanCreator is a single file application, easy to download and install; there are no separate shared libraries or support files or directories. Precompiled binaries are available (see below), but it's also easy to build LeanCreator from scratch with no other requirements than a C++11 compatible compiler.

LeanCreator is based on the code of [Qt Creator 3.6.1](https://download.qt.io/archive/qtcreator/3.6/3.6.1/qt-creator-opensource-src-3.6.1.tar.gz), mostly because I already know the internals due to [VerilogCreator](https://github.com/rochus-keller/VerilogCreator/) and it's [QtcVerilog](https://github.com/rochus-keller/QtcVerilog/) IDE, and because I'm still using Qt Creator 3.4 most of the time which does everything I need and is even faster than later versions.

In combination with LeanQt this is a big codebase and as such also a good test case for the BUSY build system; the [cloc tool](http://cloc.sourceforge.net) reports 351 kSLOC for LeanCreator and 993 kSLOC for LeanQt (1.34 mmSLOC in total).

Here is a screenshot:

![LeanCreator Screenshot](http://software.rochus-keller.ch/leancreator-2023-02-22-screenshot.png)


### Planned or work-in-progress features

The current version was successfully compiled and tested on Linux x86 & x86_64 (GCC), macOS x86_64 & M1 (CLANG), and Windows x86 & AMD64 (MSVC & MinGW). The tests included navigating the LeanQt source tree with different parameter settings, and running some of the examples in the debugger with breakpoints and value inspections. 

On Windows LeanCreator was successfully tested with MSVC 2013, 2015 and 2022 (with CDB), and also with [MinGW 12.2.0 i686](https://github.com/niXman/mingw-builds-binaries/releases/download/12.2.0-rt_v10-rev2/i686-12.2.0-release-win32-dwarf-msvcrt-rt_v10-rev2.7z) (with GDB). Debugging seems to even work better than with MSVC, where CDB is very slow on symbol load. Clang on Windows seems to work, but there are tons of compiler errors when compiling LeanQt. Lldb on Windows seems not to work however.

The new multi-core BUSY builder is extremely fast. Building LeanCreator on the eight cores of Mac M1 takes less than 10 minutes. Also finding out whether something has to be recompiled is only a split second (compared to several minutes with Qt Creator and make). Also the header dependency tracking is very fast; it is based on the built-in C++ parser used for code indexing. All in all LeanCreator builds much faster than Qt Creator with qmake or cmake even without the Ninja backend; the latter is therefore no longer a top priority.


- [x] Basic, stand-alone application with statically linked plugins
- [x] C++ support
- [x] rebranding
- [x] deep BUSY integration (replacement for qmake)
- [x] GCC, Clang and MSVC support 
- [x] GDB, LLDB and CDB support 
- [x] Extend BUSY file navigation
- [x] BUSY multi-core parallel builder
- [x] BUSY builds with header dependency tracking
- [x] Other convenience features
- [x] Wizards for project and code file creation
- [x] Stripped-down help integration
- [x] Support current LLDB versions (minimal support), see NOTE

NOTE: 

- Tests with LLDB 1316 on Mac M1, LLDB 350 on Mac x86_64 and LLDB 11 on Linux Debian 11 x86_64 were successful.
- On Mac x86_64 and on Linux you can force the old LldbEngine1 or the new LldbEngine2 to be used by setting the environment variable to either "1" or "2"; please tell me with which LLDB version and engine combinations on which platform you were successful or not.
- No tests were successful with LLDB on Windows so far; Python has to be in the path so that LeanCreator recognizes the LLDB version at all; but then there are still strange LLDB error messages I didn't see on Linux or Mac and most features don't work (e.g. breakpoints, break on start, etc.).


### Long term plan

- [ ] Display BUSY file calculated values, gray-out inactives
- [ ] Lua automation
- [ ] Designer integration
- [ ] Support current LLDB versions (full support)
- [ ] Oberon+, Verilog and Lola integration
- [ ] Ninja builds


### No support planned

- qmake
- qml, quick, JS, Python
- animation or graphics effects, all stuff not supported by LeanQt
- modeleditor
- remote access via ssh
- android, ios, qnx and winrt plugins
- make, autotools, cmake and qbs 
- version control plugins
- emacs or vim simulation

#### Precompiled versions

The following precompiled versions are available at this time:

- [Windows x86](http://software.rochus-keller.ch/leancreator_windows_x86.zip)
- [Windows x86_64](http://software.rochus-keller.ch/leancreator_windows_x64.zip)
- [Linux x86](http://software.rochus-keller.ch/leancreator_linux_x86.tar.gz)
- [Linux x86_64](http://software.rochus-keller.ch/leancreator_linux_x64.tar.gz)
- [Mac x86_64](http://software.rochus-keller.ch/leancreator_macos_x64.zip)
- [Mac M1](http://software.rochus-keller.ch/leancreator_macos_m1.zip)

Just download, unzip and run; no installation required; it's just a single executable.

On Mac the terminal opens when LeanCreator is run, and the menus are only active if the application was in the background one time; to avoid this the application can be included in an application bundle. Also note that the application on Mac must be started via the "open" command from the context menu; otherwise the system refuses to start the app.

NOTE that the Windows versions are compiled with MT using a statically linked C/C++ runtime, so no Microsoft runtime has to be installed. The executable runs even on Windows 7.

For convenience here are the [pre-built Qt 5.6.3 help files (*.qch)](http://software.rochus-keller.ch/Qt-5.6.3-qch.zip) compatible with LeanCreator and LeanQt (download, unpack and reference with LeanCreator/Options/Documentation).

### How to build LeanCreator

To build LeanCreator using LeanQt and the BUSY build system (with no other dependencies than a C++11 compiler), do the following:

1. Create a new directory; we call it the root directory here
1. Download https://github.com/rochus-keller/LeanQt/archive/refs/heads/master.zip and unpack it to the root directory; rename the resulting directory to "LeanQt".
1. Download https://github.com/rochus-keller/LeanCreator/archive/refs/heads/master.zip and unpack it to the root directory; rename the resulting directory to "LeanCreator".
1. Download https://github.com/rochus-keller/BUSY/archive/refs/heads/master.zip and unpack it to the root directory; rename the resulting directory to "BUSY".
1. Open a command line in the build directory and type `cc *.c -O2 -lm -O2 -o lua` or `cl /O2 /MD /Fe:lua.exe *.c` depending on whether you are on a Unix or Windows machine; wait a few seconds until the Lua executable is built.
1. Now type `./lua build.lua ../LeanCreator` (or `lua build.lua ../LeanCreator` on Windows); wait until the LeanCreator executable is built (about an hour); you find it in the output/app subdirectory.

NOTE that if you build on Windows you have to first open a console and run vcvars32.bat or vcvars64.bat provided e.g. by VisualStudio (see e.g. [here](https://learn.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-170) for more information).

### Additional Credits

- Copyright (C) 2016 by The Qt Company Ltd. 
- Copyright (C) 2008-2011 Nokia Corporation and/or its subsidiary(-ies).
- Copyright (C) 2007 Trolltech AS.

### Support

If you need support or would like to post issues or feature requests please use the Github issue list at https://github.com/rochus-keller/LeanCreator/issues or send an email to the author.

