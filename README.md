# libQMatrixClient

[![license](https://img.shields.io/github/license/QMatrixClient/libqmatrixclient.svg)](https://github.com/QMatrixClient/libqmatrixclient/blob/master/COPYING)
![status](https://img.shields.io/badge/status-beta-yellow.svg)
[![release](https://img.shields.io/github/release/QMatrixClient/libqmatrixclient/all.svg)](https://github.com/QMatrixClient/libqmatrixclient/releases/latest)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/1023/badge)](https://bestpractices.coreinfrastructure.org/projects/1023)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg?style=flat-square)](http://makeapullrequest.com)

libQMatrixClient is a Qt5-based library to make IM clients for the [Matrix](https://matrix.org) protocol. It is the backbone of [Quaternion](https://github.com/QMatrixClient/Quaternion), [Tensor](https://matrix.org/docs/projects/client/tensor.html) and some other projects.

## Contacts
You can find authors of libQMatrixClient in the Matrix room: [#qmatrixclient:matrix.org](https://matrix.to/#/#qmatrixclient:matrix.org).

You can also file issues at [the project's issue tracker](https://github.com/QMatrixClient/libqmatrixclient/issues). If you have what looks like a security issue, please see respective instructions in CONTRIBUTING.md.

## Building and usage
So far the library is typically used as a git submodule of another project (such as Quaternion); however it can be built separately (either as a static or as a dynamic library). As of version 0.2, the library can be installed and CMake package config files are provided; projects can use `find_package(QMatrixClient)` to setup their code with the installed library files. PRs to enable the same for qmake are most welcome.

The source code is hosted at GitHub: https://github.com/QMatrixClient/libqmatrixclient - checking out a certain commit or tag from GitHub (rather than downloading the archive) is the recommended way for one-off building. If you want to hack on the library as a part of another project (e.g. you are working on Quaternion but need to do some changes to the library code), you're advised to make a recursive check out of that project (in this case, Quaternion) and update the library submodule to its master branch.

Tags starting with `v` represent released versions; `rc` mark release candidates.

### Pre-requisites
- a Linux, macOS or Windows system (desktop versions tried; Ubuntu Touch is known to work; mobile Windows and iOS might work too but never tried)
  - For Ubuntu flavours - zesty or later (or a derivative) is good enough out of the box; older ones will need PPAs at least for a newer Qt; in particular, if you have xenial you're advised to add Kubuntu Backports PPA for it
- a Git client to check out this repo
- Qt 5 (either Open Source or Commercial), version 5.6 or higher
- a build configuration tool:
  - CMake (from your package management system or [the official website](https://cmake.org/download/))
  - or qmake (comes with Qt)
- a C++ toolchain supported by your version of Qt (see a link for your platform at [the Qt's platform requirements page](http://doc.qt.io/qt-5/gettingstarted.html#platform-requirements))
  - GCC 5 (Windows, Linux, macOS), Clang 5 (Linux), Apple Clang 8.1 (macOS) and Visual C++ 2015 (Windows) are the oldest officially supported; Clang 3.8 and GCC 4.9.2 are known to still work, maintenance patches for them are accepted
  - any build system that works with CMake and/or qmake should be fine: GNU Make, ninja (any platform), NMake, jom (Windows) are known to work.

#### Linux
Just install things from the list above using your preferred package manager. If your Qt package base is fine-grained you might want to run cmake/qmake and look at error messages. The library is entirely offscreen (QtCore and QtNetwork are essential) but it also depends on QtGui in order to handle avatar thumbnails.

#### macOS
`brew install qt5` should get you a recent Qt5. If you plan to use CMake, you will need to tell it about the path to Qt by passing `-DCMAKE_PREFIX_PATH=$(brew --prefix qt5)`

#### Windows
1. Install Qt5, using their official installer.
1. If you plan to build with CMake, install CMake; if you're ok with qmake, you don't need to install anything on top of Qt. The commands in further sections imply that cmake/qmake is in your PATH - otherwise you have to prepend those commands with actual paths. As an option, it's a good idea to run a `qtenv2.bat` script that can be found in `C:\Qt\<Qt version>\<toolchain>\bin` (assuming you installed Qt to `C:\Qt`); the only thing it does is adding necessary paths to PATH. You might not want to run that script on system startup but it's very handy to setup the environment before building. For CMake, setting `CMAKE_PREFIX_PATH` in the same way as for macOS (see above), also helps.

There are no official MinGW-based 64-bit packages for Qt. If you're determined to build a 64-bit library, either use a Visual Studio toolchain or build Qt5 yourself as described in Qt documentation.

### Building
#### CMake-based
In the root directory of the project sources:
```
mkdir build_dir
cd build_dir
cmake .. # Pass -DCMAKE_PREFIX_PATH and -DCMAKE_INSTALL_PREFIX here if needed
cmake --build . --target all
```
This will get you the compiled library in `build_dir` inside your project sources. Static builds are tested on all supported platforms. Dynamic builds of libqmatrixclient are only tested on Linux at the moment; experiments with dynamic builds on Windows/macOS are welcome. Taking a look at [qmc-example](https://github.com/QMatrixClient/libqmatrixclient/tree/master/examples) (used to test the library) should give you a basic idea of using libQMatrixClient; for more extensive usage check out the source code of [Quaternion](https://github.com/QMatrixClient/Quaternion) (the reference client built on QMatrixClient).

You can install the library with CMake:
```
cmake --build . --target install
```
This will also install cmake package config files; once this is done, you can use `examples/CMakeLists.txt` to compile the example with the _installed_ library. This file is a good starting point for your own CMake-based project using libQMatrixClient.
Installation of `qmc-example` application can be skipped by setting `QMATRIXCLIENT_INSTALL_EXAMPLE` to `OFF`.

#### qmake-based
The library provides a .pri file with an intention to be included from a bigger project's .pro file. As a starting point you can use `qmc-example.pro` that will build a minimal example of library usage for you. In the root directory of the project sources:
```
qmake qmc-example.pro
make all
```
This will get you `debug/qmc-example` and `release/qmc-example` console executables that login to the Matrix server at matrix.org with credentials of your choosing (pass the username and password as arguments), run a sync long-polling loop and do some tests of the library API.

Installing the library with qmake is not possible; similarly, a .prl file is not provided. A PR to fix this is welcome.

## Troubleshooting

#### Building fails

If `cmake` fails with...
```
CMake Warning at CMakeLists.txt:11 (find_package):
  By not providing "FindQt5Widgets.cmake" in CMAKE_MODULE_PATH this project
  has asked CMake to find a package configuration file provided by
  "Qt5Widgets", but CMake did not find one.
```
...then you need to set the right `-DCMAKE_PREFIX_PATH` variable, see above.

#### Logging configuration

libqmatrixclient uses Qt's logging categories to make switching certain types of logging easier. In case of troubles at runtime (bugs, crashes) you can increase logging if you add the following to the `QT_LOGGING_RULES` environment variable:
```
libqmatrixclient.<category>.<level>=<flag>
```
where
- `<category>` is one of: `main`, `jobs`, `jobs.sync`, `events`, `events.ephemeral`, and `profiler` (you can always find the full list in the file `logging.cpp`)
- `<level>` is one of `debug` and `warning`
- `<flag>` is either `true` or `false`.

`*` can be used as a wildcard for any part between two dots, and comma is used for a separator. Latter statements override former ones, so if you want to switch on all debug logs except `jobs` you can set
```
QT_LOGGING_RULES="libqmatrixclient.*.debug=true,libqmatrixclient.jobs.debug=false"
```

#### Cache format
In case of troubles with room state and caching it may be useful to switch cache format from binary to JSON. To do that, set the following value in your client's configuration file/registry key (you might need to create the libqmatrixclient key for that): `libqmatrixclient/cache_type` to `json`. This will make cache saving and loading work slightly slower but the cache will be in a text JSON file (very long and unindented so prepare a good JSON viewer or text editor with JSON formatting capabilities).
