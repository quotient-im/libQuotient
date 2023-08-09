# libQuotient

<a href='https://matrix.org'><img src='https://matrix.org/docs/projects/images/made-for-matrix.png' alt='Made for Matrix' height=64 target=_blank /></a>

[![license](https://img.shields.io/github/license/quotient-im/libQuotient.svg)](https://github.com/quotient-im/libQuotient/blob/dev/COPYING)
![status](https://img.shields.io/badge/status-beta-yellow.svg)
[![release](https://img.shields.io/github/release/quotient-im/libQuotient/all.svg)](https://github.com/quotient-im/libQuotient/releases/latest)
[![](https://img.shields.io/cii/percentage/1023.svg?label=CII%20best%20practices)](https://bestpractices.coreinfrastructure.org/projects/1023/badge)
![](https://img.shields.io/github/commit-activity/y/quotient-im/libQuotient.svg)
![CI Status](https://img.shields.io/github/actions/workflow/status/quotient-im/libQuotient/ci.yml)
![Sonar Tech Debt](https://img.shields.io/sonar/tech_debt/quotient-im_libQuotient?server=https%3A%2F%2Fsonarcloud.io)
![Sonar Coverage](https://img.shields.io/sonar/coverage/quotient-im_libQuotient?server=https%3A%2F%2Fsonarcloud.io)
![Matrix](https://img.shields.io/matrix/quotient:matrix.org?logo=matrix)

The Quotient project aims to produce a Qt-based SDK to develop applications
for [Matrix](https://matrix.org). libQuotient is a library that enables client
applications. It is the backbone of
[Quaternion](https://github.com/quotient-im/Quaternion),
[NeoChat](https://matrix.org/docs/projects/client/neo-chat) and other projects.

## Contacts
You can find Quotient developers in the Matrix room:
[#quotient:matrix.org](https://matrix.to/#/#quotient:matrix.org).

You can file issues at
[the project issue tracker](https://github.com/quotient-im/libQuotient/issues).
If you find what looks like a security issue, please use instructions
in [SECURITY.md](./SECURITY.md).

## Getting and using libQuotient
Depending on your platform, the library can be obtained from a package
management system. Recent releases of Fedora, Debian and openSUSE already have
it. Alternatively, you can build the library from the source and bundle it with
your application, as described below.

### Pre-requisites
To use libQuotient (i.e. build or run applications with it), you'll need:
- A recent Linux, macOS or Windows system (desktop versions are known to work,
  and there's also limited positive experience with Android)
  - Recent enough Linux examples: Debian Bullseye; Fedora 35;
    openSUSE Leap 15.4; Ubuntu 22.04 LTS
- Qt 5.15 or 6 - either Open Source or Commercial
- QtKeychain (https://github.com/frankosterfeld/qtkeychain) - 0.12 or newer is
  recommended; the build configuration of both QtKeychain and libQuotient 
  must use the same Qt major version

To build applications with libQuotient, you'll also need:
- CMake 3.16 or newer
- A C++ toolchain that supports at least some subset of C++20 (concepts,
  in particular):
  - GCC 11 (Windows, Linux, macOS), Clang 11 (Linux), Apple Clang 12 (macOS)
    and Visual Studio 2019 (Windows) are the oldest officially supported
- If using E2EE (beta, as of libQuotient 0.8):
  - libolm 3.2.5 or newer (the latest 3.x strongly recommended)
  - OpenSSL (both 1.1.x and 3.x are known to work; 3.x is strongly 
    recommended but should match the version libQuotient was/is built with)
- Any build system that works with CMake should be fine; known to work are
  GNU Make and ninja (recommended) on any platform, NMake and jom on Windows

The requirements to build libQuotient itself are basically the same except
that you should install development libraries for the dependencies listed above.

#### Linux
Just install the prerequisites using your preferred package manager. If your Qt
package base is fine-grained you might want to run CMake and look at error
messages. The library is entirely offscreen; however, aside from QtCore and
QtNetwork it also depends on QtGui in order to handle avatar thumbnails, without
any on-screen drawing.

#### macOS
`brew install qt qtkeychain` should get you the most recent versions of the
runtime libraries. Note that by default it installs Qt 6 and that's the only
available configuration for QtKeychain in Homebrew. If you need to go with Qt 5,
use `brew install qt@5` and build QtKeychain from the source.

You may need to add the output of `brew --prefix qt` and
`brew --prefix qtkeychain` to `CMAKE_PREFIX_PATH` (see below) to make CMake
aware of the library locations.

If using E2EE, you need to also `brew install libolm openssl@1.1` (it is
strongly recommended to have OpenSSL of the version that was used to build Qt;
as of this writing, it's 1.1.x but check https://formulae.brew.sh/formula/qt
for the most current situation). If necessary, add path prefixes for these
two to `CMAKE_PREFIX_PATH` as well.

#### Windows
Install Qt (and OpenSSL, if using E2EE) using The Qt Project official installer;
make sure to also tick the CMake box in the list of components to install unless
you already have. This will get you both the runtime libraries and the files
necessary for building libQuotient or with libQuotient. If you go this way,
you'll have to build QtKeychain from the source code.

Alternatively, you can use vcpkg to install Qt, OpenSSL 1.1, and QtKeychain.
You're not getting Qt Creator, which is a very nice IDE to deal with Qt-based
projects; but if you already use VSCode or CLion, you might prefer this route.

_If you build from the command line_: the commands in further sections imply
that `cmake` is in your `PATH`, otherwise you have to prepend those commands
with actual paths. It's a good idea to run the `qtenv2.bat` script that can
be found in `C:\Qt\<Qt version>\<toolchain>\bin` (assuming you installed Qt to
`C:\Qt`). This script adds necessary paths to `PATH`. You might not want to run
that script on system startup but it's very handy to setup the environment
before building.

_If you use a C++ IDE_: you should be able to configure CMake path and extra
options (`CMAKE_PREFIX_PATH`, in particular) in its settings. It is recommended
NOT to add the path for Qt (or any other library) to `PATH` explicitly; use
`CMAKE_PREFIX_PATH` instead and leave `PATH` unchanged. If your IDE is
Qt Creator, you shouldn't need to deal with Qt paths at all, just pick the right
kit and go straight to building.

If you're trying out E2EE, you will also need libolm. You'll have to build it
yourself - there's no official or semi-official binary library you can download.
The source code is available at https://gitlab.matrix.org/matrix-org/olm;
and you can use the same toolchain (CMake+MSVC, e.g.) you'll use for Quotient.


## Using the library
If you're just starting a project using libQuotient from scratch, you can copy
`quotest/CMakeLists.txt` to your project and change `quotest` to your
project name. If you already have an existing CMakeLists.txt, you need to insert
a `find_package(Quotient REQUIRED)` line to an appropriate place in it (use
`find_package(Quotient)` if libQuotient is not a hard dependency for you) and
then add `Quotient` to your `target_link_libraries()` line.

Building with dynamic linkage is only tested on Linux at the moment and is
a recommended way of linking your application with libQuotient on this platform.
Static linkage is the default on Windows/macOS; feel free to experiment
with dynamic linking and provide feedback with your results. 

As for the documentation on the library - a (very basic) overview can be found
at
[the respective wiki page](https://github.com/quotient-im/libQuotient/wiki/libQuotient-overview).
Beyond that, looking at [Quotest](quotest) - the test application that comes
with libQuotient - may help you with most common use cases such as sending
messages, uploading files, setting room state etc. The Doxygen documentation
for the library can be found at https://quotient-im.github.io/libQuotient/.

For examples and patterns of more extensive usage feel free to check out (and 
copy, with appropriate attribution) the source code of
[Quaternion](https://github.com/quotient-im/Quaternion) (the reference client
for libQuotient) or [NeoChat](https://invent.kde.org/network/neochat).

Since Quotient 0.7.2, all header files of libQuotient
*except those ending with `_p.h`* are considered public and covered by API/ABI
stability guarantees. Specifically, the API and ABI are backwards compatible 
within every minor version (0.7.x releases) with every next minor version (0.
8, e.g.) breaking the compatibility. Once we reach 1.0, this rule will apply 
to the major version, aligning with [semantic versioning](https://semver.org/)
rules. `_p.h` files are not covered by these guarantees; definitions from those
should not be directly used by clients; these header files are not guaranteed
to be available from Linux distributions, in particular.


## Building the library
On platforms other than Linux you will have to build libQuotient yourself
before usage - nobody packaged it so far (contributions welcome!). You may also
want to build the library on Linux if you need an unreleased snapshot.

[The source code is at GitHub](https://github.com/quotient-im/libQuotient).
Checking out a certain commit or tag (rather than downloading the archive)
along with submodules is strongly recommended. If you want to hack on
the library as a part of another project (e.g. you are working on Quaternion
but need to do some changes to the library code), it makes sense
to make a recursive check out of that project (in this case, Quaternion)
and update the library submodule (also recursively) within the appropriate
branch. Be mindful of API compatibility restrictions: e.g., Quaternion 0.0.95
will not build with the `dev` branch of libQuotient, only with `0.6.x` branch.

Tags consisting solely of digits and fullstops (e.g., `0.7.0`) correspond to
released versions; tags ending with `-betaN` or `-rcN` mark respective
pre-releases. If/when packaging pre-releases, it is advised to replace
the leading `-` with `~` (tilde).

libQuotient is a classic CMake-based project; assuming the dependencies are
in place, the following commands issued in the root directory of the project
sources:
```shell script
cmake -B build -S . # [-D<cmake-variable>=<value>...], see below
cmake --build build --target all
```
will get you a compiled library in `build` (make sure it exists before running).
Any C++ IDE that works with CMake should be able to do the same with minimal
configuration effort.

Static builds are tested on all supported platforms, building the library as
a shared object (aka dynamic library) is known to work on Linux and macOS,
untested on Windows.

Before proceeding, double-check that you have installed development libraries
for all prerequisites above. CMake will stop and tell you if something's missing.

The first CMake invocation above configures the build. You can pass CMake
variables (such as `-DCMAKE_PREFIX_PATH="path1;path2;..."` and
`-DCMAKE_INSTALL_PREFIX=path`) to that invocation if needed.
[CMake documentation](https://cmake.org/cmake/help/latest/index.html)
(pick the CMake version at the top of the page that you use) describes
the standard variables coming with CMake. On top of them, Quotient understands:
- `Quotient_INSTALL_TESTS=<ON/OFF>`, `ON` by default - install `quotest` along
  with the library files when `install` target is invoked. `quotest` is a small
  command-line program that (assuming correct parameters, see `quotest --help`)
  that tries to connect to a given room as a given user and perform some basic
  Matrix operations, such as sending messages and small files, redaction,
  setting room tags etc. This is useful to check the sanity of your library
  installation.
- `Quotient_ENABLE_E2EE=<ON/OFF>`, `OFF` by default - enable work-in-progress
  E2EE code in the library. As of version 0.8, this code is beta-quality and
  is generally good for trying the technology but really not for
  mission-critical applications.

  Switching this on will define `Quotient_E2EE_ENABLED` macro (note
  the difference from the CMake switch) for compiler invocations on all
  Quotient and Quotient-dependent (if it uses `find_package(Quotient)`)
  code; so you can use `#ifdef Quotient_E2EE_ENABLED` to guard the code that
  depends on parts of Quotient that only get built for E2EE. Be mindful that
  the _runtime_ default for E2EE is still to be off; you should either call
  `Connection::setEncryptionDefault(true);` before creating any `Connection`
  objects in your code, or call `Connection::enableEncryption()` on each
  `Connection` object you want to enable it.
- `MATRIX_SPEC_PATH` and `GTAD_PATH` - these two variables are used to point
  CMake to the directory with the matrix-doc repository containing API files
  and to a GTAD binary. These two are used to generate C++ files from Matrix
  Client-Server API description made in OpenAPI notation. This is not needed
  if you just need to build the library; if you're really into hacking on it,
  please read the respective sections in [CONTRIBUTING.md](./CONTRIBUTING.md)
  and [CODE_GENERATION.md](./CODE_GENERATION.md).
- `QUOTIENT_FORCE_NAMESPACED_INCLUDES=<ON/OFF>`, since version 0.7.2, `OFF`
  by default (note that QUOTIENT is in caps here, unlike options above) - when
  this option is set to `ON`, CMake skips adding
  `<top-level include prefix>/Quotient/` to include paths, thereby forcing
  the client code to use `#include <Quotient/header.h>` instead of historically
  accepted `#include <header.h>`. By default this is set to `OFF` for backwards
  compatibility; eventually this default may change so it is recommended to
  at least occasionally add `-DQUOTIENT_FORCE_NAMESPACED_INCLUDES=1` to a CMake
  invocation (or set the variable in your IDE) and make sure your code uses
  prefixed `#include` paths.

You can install the library with CMake:
```shell script
cmake --build . --target install
```
This will also install cmake package config files; once this is done, you
should be able to use [`quotest/CMakeLists.txt`](quotest/CMakeLists.txt)
to compile quotest with the _installed_ library. As mentioned above,
installation of the `quotest` binary along with the rest of the library can be
skipped by setting `Quotient_INSTALL_TESTS` to `OFF`.


## Troubleshooting

#### Building fails

- If `cmake` fails with
  ```
  CMake Warning at CMakeLists.txt:11 (find_package):
    By not providing "FindQt5Widgets.cmake" in CMAKE_MODULE_PATH this project
    has asked CMake to find a package configuration file provided by
    "Qt5Widgets", but CMake did not find one.
  ```
  then you need to set the right `-DCMAKE_PREFIX_PATH` variable, see above.
  
- If `cmake` fails with a message similar to:
  ```
  CMake Error at /usr/lib64/cmake/Qt6Core/Qt6CoreVersionlessTargets.cmake:37 (message):
    Some (but not all) targets in this export set were already defined.
  
    Targets Defined: Qt::Core
  
    Targets not yet defined: Qt::CorePrivate
  ```
  then you likely have both Qt 5 and Qt 6 on your system, and your code uses
  a different major version of Qt than Quotient. Make sure you configure the
  build so that the same major Qt version is used both by libQuotient and
  your code.

- If you use GCC and get an "unknown declarator" compilation error in the file
`qtconcurrentthreadengine.h` - unfortunately, it is an actual error in Qt 5.15
sources, see https://bugreports.qt.io/browse/QTBUG-90568 (or
https://bugreports.qt.io/browse/QTBUG-91909). The Qt company did not make
an open source release with the fix, therefore:

  - if you're on Linux - try to use Qt from your package management system, as
    most likely this bug is already fixed in the packages
  - if you're on Windows, or if you have to use Qt (5.15) from download.qt.io
    for any other reason, you should apply the fix to Qt sources: locate
    the file (the GCC error message tells exactly where it is), find the line
    with the `ThreadEngineStarter` constructor definition that looks like this:
    ```cplusplus
    ThreadEngineStarter<void>(ThreadEngine<void> *_threadEngine)
    ```
    and remove the template specialisation from the constructor name (but not
    from `ThreadEngine`). The result should look like this:
    ```cplusplus
    ThreadEngineStarter(ThreadEngine<void> *_threadEngine)
    ```
    This will fix your build (and any other build involving QtConcurrent from
    this installation of Qt - the fix is not specific to Quotient in any way).

#### Logging configuration

libQuotient uses Qt's logging categories to make switching certain types of
logging easier. In case of troubles at runtime (bugs, crashes) you can increase
logging if you add the following to the `QT_LOGGING_RULES` environment variable:
```
quotient.<category>.<level>=<flag>
```
where
- `<category>` is one of: `main`, `jobs`, `jobs.sync`, `jobs.thumbnail`,
  `events`, `events.state` (covering both the "usual" room state and account
  data), `events.members`, `events.messages`, `events.ephemeral`, `database`,
  `network`, `e2ee` and `profiler` - you can always find the full list in
  `Quotient/logging_categories_p.h`;
- `<level>` is one of `debug`, `info`, and `warning`;
- `<flag>` is either `true` or `false`.

You can use `*` (asterisk) as a wildcard for any part between two dots, and
semicolon is used for a separator. Latter statements override former ones, so
if you want to switch on all debug logs except `jobs` you can set
```shell script
QT_LOGGING_RULES="quotient.*.debug=true;quotient.jobs.debug=false"
```

(Thanks to [@eang:matrix.org](https://matrix.to/#/@eang:matrix.org]) for
contributing the original libQuotient code for logging categories.)

You may also want to set `QT_MESSAGE_PATTERN` to make logs slightly more
informative (see https://doc.qt.io/qt-6/qtlogging.html#qSetMessagePattern
for the format description). To give an example, here's what one of the library
developers uses for `QT_MESSAGE_PATTERN`:
```
`%{time h:mm:ss.zzz}|%{category}|%{if-debug}D%{endif}%{if-info}I%{endif}%{if-warning}W%{endif}%{if-critical}C%{endif}%{if-fatal}F%{endif}|%{message}`
```
(the scary `%{if}`s are just encoding the logging level into its initial letter).

#### Cache format
In case of troubles with room state and caching it may be useful to switch
cache format from binary CBOR to plaintext JSON. To do that, set
`libQuotient/cache_type` key in your client's configuration file/registry 
to `json` (you might need to create the libQuotient group as it's the only
recognised key in it so far). This will make cache saving and loading work 
slightly slower but the cache will be in text JSON files (very long and with 
no indentation; prepare a good JSON viewer or text editor with JSON 
formatting capabilities).
