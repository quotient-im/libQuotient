# Libqmatrixclient
libqmatrixclient is a Qt-based library to make IM clients for the [Matrix](https://matrix.org) protocol. It is used by the Quaternion client and is a part of the Quaternion project. The below instructions are the same for Quaternion and libqmatrixclient (the source tree of Quaternion has most up-to-date instructions but this source tree strives to closely follow).

## Contacts
You can find authors of libqmatrixclient in the Quaternion Matrix room: [#quaternion:matrix.org](https://matrix.to/#/#quaternion:matrix.org).

Issues should be submitted to [the project's issue tracker](https://github.com/Fxrh/libqmatrixclient/issues). We do not guarantee a response but we usually try to at least acknowledge the issue


## Pre-requisites
 a Linux, MacOS or Windows system (desktop versions tried; mobile Linux/Windows might work too)
- a Git client (to check out this repo)
- CMake (from your package management system or [the official website](https://cmake.org/download/))
- Qt 5 (either Open Source or Commercial), version 5.2.1 or higher as of this writing (check the CMakeLists.txt for most up-to-date information). Qt 5.3 or higher recommended on Windows.
- a C++ toolchain supported by your version of Qt (see a link for your platform at [the Qt's platform requirements page](http://doc.qt.io/qt-5/gettingstarted.html#platform-requirements))
  - GCC 4.8, Clang 3.5.0, Visual C++ 2015 are the oldest officially supported as of this writing

## Installing pre-requisites
### Linux
Just install things from "Pre-requisites" using your preferred package manager. If your Qt package base is fine-grained you might want to take a look at `CMakeLists.txt` to figure out which specific libraries libqmatrixclient uses (or blindly run cmake and look at error messages).

### OS X
`brew install qt5` should get you Qt5. You may need to tell CMake about the path to Qt by passing `-DCMAKE_PREFIX_PATH=<where-Qt-installed>`

### Windows
1. Install a Git client and CMake. The commands here imply that git and cmake are in your PATH - otherwise you have to prepend them with your actual paths.
1. Install Qt5, using their official installer. If for some reason you need to use Qt 5.2.1, select its Add-ons component in the installer as well; for later versions, no extras are needed. If you don't have a toolchain and/or IDE, you can easily get one by selecting Qt Creator and at least one toolchain under Qt Creator.
1. Make sure CMake knows about Qt and the toolchain - the easiest way is to run a qtenv2.bat script that can be found in `C:\Qt\<Qt version>\<toolchain>\bin` (assuming you installed Qt to `C:\Qt`). The only thing it does is adding necessary paths to PATH - you might not want to run it on system startup but it's very handy to setup environment before building. Setting CMAKE_PREFIX_PATH, the same way as for OS X (see above), also helps.

There are no official MinGW-based 64-bit packages for Qt. If you're determined to build 64-bit libqmatrixclient, either use a Visual Studio toolchain or build Qt5 yourself as described in Qt documentation.

## Source code
To get all necessary sources, simply clone the GitHub repo. If you have cloned Quaternion or Tensor sources with `--recursive`, you already have libqmatrixclient in the respective `lib` subdirectory.

## Building
In the root directory of the project sources:
```
mkdir build_dir
cd build_dir
cmake .. # Pass -DCMAKE_PREFIX_PATH and -DCMAKE_INSTALL_PREFIX here if needed
cmake --build . --target all
```
This will get you the compiled library in `build_dir` inside your project sources. Only static builds of libqmatrixclient are tested at the moment; experiments with dynamic builds are welcome. The two known projects to link with libqmatrixclient are Tensor and Quaternion; you should take a look at their source code before doing anything with libqmatrixclient on your own.

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