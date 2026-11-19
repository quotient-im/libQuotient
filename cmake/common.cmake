# SPDX-FileCopyrightText: The Quotient Project Contributors
#
# SPDX-License-Identifier: LGPL-3.0-or-later

# Shared CMake setup for libQuotient and quotest (in-tree or standalone).
# Include after project().

set(QtMinVersion "6.8")
string(REGEX REPLACE "^(.).*" "Qt\\1" Qt ${QtMinVersion}) # makes "Qt<MajorVersionDigit>"
set(QUOTIENT_LIB_NAME "Quotient${Qt}")

include(CheckCXXCompilerFlag)
include(FeatureSummary)
set(CMAKE_AUTOMOC ON)

# On Windows the executable and its dependent DLLs are kept together in the
# prefix root; elsewhere we use the standard GNU layout.
if (WIN32)
    if (NOT CMAKE_INSTALL_LIBDIR)
        set(CMAKE_INSTALL_LIBDIR ".")
        set(CMakeFilesLocation "cmake")
    else()
        set(CMakeFilesLocation "${CMAKE_INSTALL_LIBDIR}/cmake/${QUOTIENT_LIB_NAME}")
    endif()

    if (NOT CMAKE_INSTALL_BINDIR)
        set(CMAKE_INSTALL_BINDIR ".")
    endif()

    if (NOT CMAKE_INSTALL_INCLUDEDIR)
        set(CMAKE_INSTALL_INCLUDEDIR "include")
    endif()
else()
    include(GNUInstallDirs)
    set(CMakeFilesLocation "${CMAKE_INSTALL_LIBDIR}/cmake/${QUOTIENT_LIB_NAME}")
endif(WIN32)
