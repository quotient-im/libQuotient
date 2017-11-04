# Libqmatrixclient

[![license](https://img.shields.io/github/license/QMatrixClient/libqmatrixclient.svg)](https://github.com/QMatrixClient/libqmatrixclient/blob/master/COPYING)
![status](https://img.shields.io/badge/status-beta-yellow.svg)
[![release](https://img.shields.io/github/release/QMatrixClient/libqmatrixclient/all.svg)](https://github.com/QMatrixClient/Quaternion/releases/latest)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg?style=flat-square)](http://makeapullrequest.com)

libqmatrixclient is a Qt5-based library to make IM clients for the [Matrix](https://matrix.org) protocol. It is the backbone of [Quaternion](https://github.com/QMatrixClient/Quaternion), [Tensor](https://matrix.org/docs/projects/client/tensor.html) and some other projects.

## Contacts
You can find authors of libqmatrixclient in the Quaternion Matrix room: [#qmatrixclient:matrix.org](https://matrix.to/#/#qmatrixclient:matrix.org).

You can also file issues at [the project's issue tracker](https://github.com/QMatrixClient/libqmatrixclient/issues). If you have what looks like a security issue, please see respective instructions in CONTRIBUTING.md.

## Building and usage
So far the library is typically used as a git submodule of another project (such as Quaternion); however it can be built separately (both as a static and as a dynamic library). There is no specific installation sequence outside of other projects but since it's a CMake-based project, your mileage should be fairly short (in case it's not, issues and PRs are most welcome).

The source code is hosted at GitHub: https://github.com/QMatrixClient/libqmatrixclient - checking out a certain commit or tag from GitHub (rather than downloading the archive) is the recommended way for packagers.

There are very few tags so far; there will be more, as new versions are released.

## Pre-requisites
 a Linux, MacOS or Windows system (desktop versions tried; mobile Linux/Windows might work too)
- a Git client (to check out this repo)
- Qt 5 (either Open Source or Commercial), version 5.2.1 or higher as of this writing (check `CMakeLists.txt` for most up-to-date information)
- qmake (from the Qt 5 installation) or CMake (from your package management system or [the official website](https://cmake.org/download/)).
- a C++ toolchain supported by your version of Qt (see a link for your platform at [the Qt's platform requirements page](http://doc.qt.io/qt-5/gettingstarted.html#platform-requirements))
  - GCC 4.8, Clang 3.5.0, Visual C++ 2015 are the oldest officially supported as of this writing

#### Linux
Just install things from the list above using your preferred package manager. If your Qt package base is fine-grained you might want to take a look at `CMakeLists.txt` to figure out which specific libraries libqmatrixclient uses (or blindly run cmake and look at error messages). The library is entirely offscreen (Qt::Core and Qt::Network are essential) but it also depends on Qt::Gui in order to operate with avatar thumbnails. 

#### OS X
`brew install qt5` should get you Qt5. If you plan to use CMake, you may need to tell it about the path to Qt by passing `-DCMAKE_PREFIX_PATH=<where-Qt-installed>`

#### Windows
1. Install Qt5, using their official installer. If for some reason you need to use Qt 5.2.1, select its Add-ons component in the installer as well; for later versions, no extras are needed. If you don't have a toolchain and/or IDE, you can easily get one by selecting Qt Creator and at least one toolchain under Qt Creator. Qt 5.3 is recommended on Windows; `windeployqt` in Qt 5.2.1 is not functional enough to generate a proper list of files for installing.
1. If you plan to build with CMake, install CMake; if you're ok with qmake, you don't need to install anything on top of Qt. The commands in further sections imply that cmake/qmake is in your PATH - otherwise you have to prepend those commands with actual paths. As an option, it's a good idea to run a `qtenv2.bat` script that can be found in `C:\Qt\<Qt version>\<toolchain>\bin` (assuming you installed Qt to `C:\Qt`); the only thing it does is adding necessary paths to PATH. You might not want to run that script on system startup but it's very handy to setup the environment before building. For CMake, setting `CMAKE_PREFIX_PATH` in the same way as for OS X (see above), also helps.

There are no official MinGW-based 64-bit packages for Qt. If you're determined to build a 64-bit library, either use a Visual Studio toolchain or build Qt5 yourself as described in Qt documentation.

## Build
### CMake-based
In the root directory of the project sources:
```
mkdir build_dir
cd build_dir
cmake .. # Pass -DCMAKE_PREFIX_PATH and -DCMAKE_INSTALL_PREFIX here if needed
cmake --build . --target all
```
This will get you the compiled library in `build_dir` inside your project sources. Only static builds of libqmatrixclient are tested at the moment; experiments with dynamic builds are welcome. The two known projects to link with libqmatrixclient are Tensor and Quaternion; you should take a look at their source code before doing anything with libqmatrixclient on your own.

### qmake-based
The library only provides a .pri file with an intention to be included from a bigger project's .pro file. As a starting point you can use `qmc-example.pro` that will build a minimal example of library usage for you. In the root directory of the project sources:
```
qmake qmc-example.pro
make all
```
This will get you `debug/qmc-example` and `release/qmc-example` console executables that login to the Matrix server at matrix.org with credentials of your choosing (pass the username and password as arguments) and run a sync long-polling loop, showing some information about received events.

## Troubleshooting

If `cmake` fails with...
```
CMake Warning at CMakeLists.txt:11 (find_package):
  By not providing "FindQt5Widgets.cmake" in CMAKE_MODULE_PATH this project
  has asked CMake to find a package configuration file provided by
  "Qt5Widgets", but CMake did not find one.
```
...then you need to set the right `-DCMAKE_PREFIX_PATH` variable, see above.

libqmatrixclient uses Qt's logging categories to make switching certain types of logging easier. In case of troubles at runtime (bugs, crashes) you can increase logging if you add the following to the `QT_LOGGING_RULES` environment variable:
```
libqmatrixclient.<category>.<level>=<flag>
```
where
- `<category>` is something like `main`, `jobs`, or `events` (the full list is in the file `debug.cpp`)
- `<level>` is one of `debug` and `warning`
- `<flag>` is either `true` or `false`.

`*` can be used as a wildcard for any part between two dots, and comma is used for a separator. Latter statements override former ones, so if you want to switch on all debug logs except `jobs` you can set
```
QT_LOGGING_RULES="libqmatrixclient.*.debug=true,libqmatrixclient.jobs.debug=false"
```
