# libQuotient (former libQMatrixClient)

<a href='https://matrix.org'><img src='https://matrix.org/docs/projects/images/made-for-matrix.png' alt='Made for Matrix' height=64 target=_blank /></a>

[![license](https://img.shields.io/github/license/quotient-im/libQuotient.svg)](https://github.com/quotient-im/libQuotient/blob/master/COPYING)
![status](https://img.shields.io/badge/status-beta-yellow.svg)
[![release](https://img.shields.io/github/release/quotient-im/libQuotient/all.svg)](https://github.com/quotient-im/libQuotient/releases/latest)
[![](https://img.shields.io/cii/percentage/1023.svg?label=CII%20best%20practices)](https://bestpractices.coreinfrastructure.org/projects/1023/badge)
![](https://img.shields.io/github/commit-activity/y/quotient-im/libQuotient.svg)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/quotient-im/libQuotient.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/quotient-im/libQuotient/context:cpp)
[![merge-chance-badge](https://img.shields.io/endpoint?url=https%3A%2F%2Fmerge-chance.info%2Fbadge%3Frepo%3Dquotient-im/libquotient)](https://merge-chance.info/target?repo=quotient-im/libquotient)

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
Recent releases of Debian and openSUSE, e.g., already have the package
(under the old name). If your Linux repo doesn't provide binary package
(either libqmatrixclient - older - or libquotient - newer), or you're
on Windows or macOS, your best bet is to build the library from the source
and bundle it with your application.

### Pre-requisites
- A recent Linux, macOS or Windows system (desktop versions are known to work;
  mobile operating systems where Qt is available might work too)
  - Recent enough Linux examples: Debian Buster; Fedora 28; openSUSE Leap 15;
    Ubuntu Bionic Beaver.
- Qt 5 (either Open Source or Commercial), 5.9 or higher;
  5.12 is recommended, especially if you use qmake
- A build configuration tool (CMake is recommended, qmake is supported):
  - CMake 3.10 or newer (from your package management system or
    [the official website](https://cmake.org/download/))
  - or qmake (comes with Qt)
- A C++ toolchain with _reasonably complete_ C++17 support:
  - GCC 7 (Windows, Linux, macOS), Clang 6 (Linux), Apple Clang 10 (macOS)
    and Visual Studio 2017 (Windows) are the oldest officially supported.
- Any build system that works with CMake and/or qmake should be fine:
  GNU Make, ninja (any platform), NMake, jom (Windows) are known to work.

#### Linux
Just install things from the list above using your preferred package manager. If your Qt package base is fine-grained you might want to run cmake/qmake and look at error messages. The library is entirely offscreen (QtCore and QtNetwork are essential) but it also depends on QtGui in order to handle avatar thumbnails.

#### macOS
`brew install qt5` should get you a recent Qt5. If you plan to use CMake, you will need to tell it about the path to Qt by passing `-DCMAKE_PREFIX_PATH=$(brew --prefix qt5)`

#### Windows
Install Qt5, using their official installer; if you plan to build with CMake,
make sure to tick the CMake box in the list of installed components.

The commands in further sections imply that cmake/qmake is in your PATH,
otherwise you have to prepend those commands with actual paths. As an option
it's a good idea to run a `qtenv2.bat` script that can be found in
`C:\Qt\<Qt version>\<toolchain>\bin` (assuming you installed Qt to `C:\Qt`);
the only thing it does is adding necessary paths to PATH. You might not want
to run that script on system startup but it's very handy to setup
the environment before building. For CMake you can alternatively point
`CMAKE_PREFIX_PATH` to your Qt installation and leave PATH unchanged; but
in that case you'll have to supply the full path to CMake when calling it.

### Using the library
If you use CMake, `find_package(Quotient)` sets up the client code to use
libQuotient, assuming the library development files are installed. There's no
documented procedure to use a preinstalled library with qmake; consider
introducing a submodule in your source tree and build it along with the rest
of the application for now. Note also that qmake is considered for phase-out
in Qt 6 so you should probably think of moving over to CMake eventually.

Building with dynamic linkage is only tested on Linux at the moment and is
a recommended way of linking your application with libQuotient on this platform.
Static linkage is the default on Windows/macOS; feel free to experiment
with dynamic linking and submit PRs if you get reusable results.

[Quotest](quotest), the test application that comes with libQuotient, includes
most common use cases such as sending messages, uploading files,
setting room state etc.; for more extensive usage check out the source code
of [Quaternion](https://github.com/quotient-im/Quaternion)
(the reference client of Quotient) or [Spectral](https://gitlab.com/b0/spectral).

To ease the first step, `quotest/CMakeLists.txt` is a good starting point
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
cmake .. # [-D<cmake-variable>=<value>...], see below
cmake --build . --target all
```
This will get you the compiled library in `build_dir` inside your project
sources. Static builds are tested on all supported platforms, building
the library as a shared object (aka dynamic library) is supported on Linux
and macOS but is very likely to be broken on Windows.

The first CMake invocation configures the build. You can pass CMake variables,
such as `-DCMAKE_PREFIX_PATH="path1;path2;..."` and
`-DCMAKE_INSTALL_PREFIX=path` here if needed.
[CMake documentation](https://cmake.org/cmake/help/latest/index.html)
(pick the CMake version at the top of the page that you use) describes
the standard variables coming with CMake. On top of them, Quotient introduces:
- `Quotient_INSTALL_TESTS=<ON/OFF>`, `ON` by default - install `quotest` along
  with the library files when `install` target is invoked. `quotest` is a small
  command-line program that (assuming correct parameters, see `quotest --help`)
  that tries to connect to a given room as a given user and perform some basic
  Matrix operations, such as sending messages and small files, redaction,
  setting room tags etc. This is useful to check the sanity of your library
  installation. As of now, `quotest` expects the used homeserver to be able
  to get the contents of `#quotient:matrix.org`; this is being fixed in
  [#401](https://github.com/quotient-im/libQuotient/issues/401).
- `Quotient_ENABLE_E2EE=<ON/OFF>`, `OFF` by default - enable work-in-progress
  E2EE code in the library. As of 0.6, this code is very incomplete and leaks
  memory; only set this to `ON` if you want to help making this code work.
  Switching this on will define `Quotient_E2EE_ENABLED` macro (note
  the difference from the CMake switch) for compiler invocations on all
  Quotient and Quotient-dependent (if it uses `find_package(Quotient 0.6)`)
  code; so you can use `#ifdef Quotient_E2EE_ENABLED` to guard the code using
  E2EE parts of Quotient.
- `MATRIX_DOC_PATH` and `GTAD_PATH` - these two variables are used to point
  CMake to the directory with the matrix-doc repository containing API files
  and to a GTAD binary. These two are used to generate C++ files from Matrix
  Client-Server API description made in OpenAPI notation. This is not needed
  if you just need to build the library; if you're really into hacking on it,
  CONTRIBUTING.md elaborates on what these two variables are for.

You can install the library with CMake:
```shell script
cmake --build . --target install
```
This will also install cmake package config files; once this is done, you
should be able to use [`quotest/CMakeLists.txt`](quotest/CMakeLists.txt) to compile quotest
with the _installed_ library. Installation of the `quotest` binary
along with the rest of the library can be skipped
by setting `Quotient_INSTALL_TESTS` to `OFF`.

### qmake-based
The library provides a .pri file with an intention to be included from a bigger project's .pro file. As a starting point you can use `quotest.pro` that will build a minimal example of library usage for you. In the root directory of the project sources:
```shell script
qmake quotest.pro
make all
```
This will get you `debug/quotest` and `release/quotest`
console executables that login to the Matrix server at matrix.org with
credentials of your choosing (pass the username and password as arguments),
run a sync long-polling loop and do some tests of the library API. Note that
qmake didn't really know about C++17 until Qt 5.12 so if your Qt is older
you may have quite a bit of warnings during the compilation process.

Installing the standalone library with qmake is not implemented yet; PRs are
welcome though.

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
- `<category>` is one of: `main`, `jobs`, `jobs.sync`, `jobs.thumbnail`,
  `events`, `events.state` (covering both the "usual" room state and account
  data), `events.messages`, `events.ephemeral`, `e2ee` and `profiler` (you can
  always find the full list in `lib/logging.cpp`);
- `<level>` is one of `debug`, `info`, and `warning`;
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
the libQuotient key for that): `libQuotient/cache_type` to `json`.
This will make cache saving and loading work slightly slower but the cache
will be in text JSON files (possibly very long and unindented so prepare a good
JSON viewer or text editor with JSON formatting capabilities).
