# libQuotient (former libQMatrixClient)

<a href='https://matrix.org'><img src='https://matrix.org/docs/projects/images/made-for-matrix.png' alt='Made for Matrix' height=64 target=_blank /></a>

[![license](https://img.shields.io/github/license/quotient-im/libQuotient.svg)](https://github.com/quotient-im/libQuotient/blob/master/COPYING)
![status](https://img.shields.io/badge/status-beta-yellow.svg)
[![release](https://img.shields.io/github/release/quotient-im/libQuotient/all.svg)](https://github.com/quotient-im/libQuotient/releases/latest)
[![](https://img.shields.io/cii/percentage/1023.svg?label=CII%20best%20practices)](https://bestpractices.coreinfrastructure.org/projects/1023/badge)
![](https://img.shields.io/github/commit-activity/y/quotient-im/libQuotient.svg)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg?style=flat-square)](http://makeapullrequest.com)

The Quotient project aims to produce a Qt5-based SDK to develop applications
for [Matrix](https://matrix.org). libQuotient is a library that enables client
applications. It is the backbone of
[Quaternion](https://github.com/quotient-im/Quaternion),
[Spectral](https://matrix.org/docs/projects/client/spectral.html) and
other projects.
Versions 0.5.x and older use the previous name - libQMatrixClient.

## Contacts
You can find Quotient developers in the Matrix room:
[#quotient:matrix.org](https://matrix.to/#/#quotient:matrix.org).

You can file issues at
[the project issue tracker](https://github.com/quotient-im/libQuotient/issues).
If you find what looks like a security issue, please use instructions
in SECURITY.md.

## Getting and using libQuotient
Depending on your platform, the library can come as a separate package.
Recent releases of Debian and OpenSuSE, e.g., already have the package
(under the old name). If your Linux repo doesn't provide binary package
(either libqmatrixclient - older - or libquotient - newer), or you're
on Windows or macOS, your best bet is to build the library from the source
and bundle it with your application. In

### Pre-requisites
- A Linux, macOS or Windows system (desktop versions tried; Ubuntu Touch
  is known to work; mobile Windows and iOS might work too but never tried)
  - For Ubuntu flavours - zesty or later is good enough out of the box;
    older ones will need PPAs at least for a newer Qt. In particular,
    if you (still) have xenial and cannot upgrade to a newer release
    you'll have to add Kubuntu Backports PPA for it.
- Qt 5 (either Open Source or Commercial), 5.9 or higher.
- A build configuration tool:
  - CMake (from your package management system or
    [the official website](https://cmake.org/download/))
  - or qmake (comes with Qt)
- A C++ toolchain with C++14 support
  - GCC 5 (Windows, Linux, macOS), Clang 5 (Linux), Apple Clang 8.1 (macOS)
    and Visual Studio 2017 (Windows) are the oldest officially supported;
    Clang 3.8, GCC 4.9.2, VS 2015 may work but not actively maintained.
- Any build system that works with CMake and/or qmake should be fine:
  GNU Make, ninja (any platform), NMake, jom (Windows) are known to work.

#### Linux
Just install things from the list above using your preferred package manager. If your Qt package base is fine-grained you might want to run cmake/qmake and look at error messages. The library is entirely offscreen (QtCore and QtNetwork are essential) but it also depends on QtGui in order to handle avatar thumbnails.

#### macOS
`brew install qt5` should get you a recent Qt5. If you plan to use CMake, you will need to tell it about the path to Qt by passing `-DCMAKE_PREFIX_PATH=$(brew --prefix qt5)`

#### Windows
1. Install Qt5, using their official installer.
1. If you plan to build with CMake, install CMake; if you're ok with qmake, you don't need to install anything on top of Qt. The commands in further sections imply that cmake/qmake is in your PATH - otherwise you have to prepend those commands with actual paths. As an option, it's a good idea to run a `qtenv2.bat` script that can be found in `C:\Qt\<Qt version>\<toolchain>\bin` (assuming you installed Qt to `C:\Qt`); the only thing it does is adding necessary paths to PATH. You might not want to run that script on system startup but it's very handy to setup the environment before building. For CMake, setting `CMAKE_PREFIX_PATH` in the same way as for macOS (see above), also helps.

### Using the library
If you use CMake, `find_package(Quotient)` sets up the client code to use
libQuotient, assuming the library development files are installed. There's no
documented procedure to use a preinstalled library with qmake; consider
introducing a submodule in your source tree and build it along with the rest
of the application for now. Patches to provide .prl files for qmake
are welcome.

Building with dynamic linkage are only tested on Linux at the moment and are
a recommended way of linking your application with libQuotient on this platform.
Feel free 
Static linkage is the default on Windows/macOS; feel free to experiment
with dynamic linking and submit PRs if you get reusable results.

The example/test application that comes with libQuotient,
[qmc-example](https://github.com/quotient-im/libQuotient/tree/master/examples)
includes most common use cases such as sending messages, uploading files,
setting room state etc.; for more extensive usage check out the source code
of [Quaternion](https://github.com/quotient-im/Quaternion)
(the reference client of Quotient) or [Spectral](https://gitlab.com/b0/spectral).

To ease the first step, `examples/CMakeLists.txt` is a good starting point
for your own CMake-based project using libQuotient.

## Building the library
[The source code is at GitHub](https://github.com/quotient-im/libQuotient).
Checking out a certain commit or tag (rather than downloading the archive)
along with submodules is strongly recommended. If you want to hack on
the library as a part of another project (e.g. you are working on Quaternion
but need to do some changes to the library code), it makes sense
to make a recursive check out of that project (in this case, Quaternion)
and update the library submodule (also recursively) to its master branch.

Tags consisting of digits and periods represent released versions; tags ending
with `-betaN` or `-rcN` mark pre-releases. If/when packaging pre-releases,
it is advised to replace a dash with a tilde.

### CMake-based
In the root directory of the project sources:
```shell script
mkdir build_dir
cd build_dir
cmake .. # Pass -DCMAKE_PREFIX_PATH and -DCMAKE_INSTALL_PREFIX here if needed
cmake --build . --target all
```
This will get you the compiled library in `build_dir` inside your project
sources. Static builds are tested on all supported platforms. 

You can install the library with CMake:
```shell script
cmake --build . --target install
```
This will also install cmake package config files; once this is done, you
should be able to use `examples/CMakeLists.txt` to compile qmc-example
with the _installed_ library. Installation of the `qmc-example` binary
along with the rest of the library can be skipped
by setting `QMATRIXCLIENT_INSTALL_EXAMPLE` to `OFF`.

### qmake-based
The library provides a .pri file with an intention to be included from a bigger project's .pro file. As a starting point you can use `qmc-example.pro` that will build a minimal example of library usage for you. In the root directory of the project sources:
```shell script
qmake qmc-example.pro
make all
```
This will get you `debug/qmc-example` and `release/qmc-example` console executables that login to the Matrix server at matrix.org with credentials of your choosing (pass the username and password as arguments), run a sync long-polling loop and do some tests of the library API.

Installing the standalone library with qmake is not implemented yet.

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

libQuotient uses Qt's logging categories to make switching certain types of logging easier. In case of troubles at runtime (bugs, crashes) you can increase logging if you add the following to the `QT_LOGGING_RULES` environment variable:
```
quotient.<category>.<level>=<flag>
```
where
- `<category>` is one of: `main`, `jobs`, `jobs.sync`, `events`, `events.ephemeral`, and `profiler` (you can always find the full list in the file `lib/logging.cpp`)
- `<level>` is one of `debug` and `warning`
- `<flag>` is either `true` or `false`.

`*` can be used as a wildcard for any part between two dots, and semicolon is used for a separator. Latter statements override former ones, so if you want to switch on all debug logs except `jobs` you can set
```shell script
QT_LOGGING_RULES="quotient.*.debug=true;quotient.jobs.debug=false"
```
Note that `quotient` is a prefix that only works since version 0.6 of
the library; 0.5.x and older used `libqmatrixclient` instead. If you happen
to deal with both libQMatrixClient-era and Quotient-era versions,
it's reasonable to use both prefixes, to make sure you're covered with no
regard to the library version. For example, the above setting could look like
```shell script
QT_LOGGING_RULES="libqmatrixclient.*.debug=true;libqmatrixclient.jobs.debug=false;quotient.*.debug=true;quotient.jobs.debug=false"
```

#### Cache format
In case of troubles with room state and caching it may be useful to switch
cache format from binary to JSON. To do that, set the following value in
your client's configuration file/registry key (you might need to create
the libqmatrixclient key for that): `libqmatrixclient/cache_type` to `json`.
This will make cache saving and loading work slightly slower but the cache
will be in a text JSON file (very long and unindented so prepare a good
JSON viewer or text editor with JSON formatting capabilities).
