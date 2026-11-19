# SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
#
# SPDX-License-Identifier: BSD-3-Clause

# Quotest can be built in two ways:
#  * in-tree, as a subdirectory of the libQuotient build
#  * standalone, by configuring this directory directly against an installed libQuotient
# For standalone, we have to set up things the parent project would have provided, so that
# the rest of this file is identical in both cases.
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.26)
    project(quotest LANGUAGES CXX)

    set(QtMinVersion "6.8")
    string(REGEX REPLACE "^(.).*" "Qt\\1" Qt ${QtMinVersion})
    find_package(${Qt} ${QtMinVersion} REQUIRED COMPONENTS Core Test Concurrent)
    find_package(QuotientQt6 REQUIRED)
    set(QUOTIENT_LIB_NAME QuotientQt6)

    set(CMAKE_AUTOMOC ON)
    include(CheckCXXCompilerFlag)
    include(FeatureSummary)
    include(GNUInstallDirs)
endif()

set(quotest_SRCS quotest.cpp)

find_package(${Qt} COMPONENTS Concurrent)
add_executable(quotest ${quotest_SRCS})
target_link_libraries(quotest PRIVATE ${Qt}::Core ${Qt}::Test ${Qt}::Concurrent ${QUOTIENT_LIB_NAME})
set_target_properties(quotest PROPERTIES
    CXX_STANDARD 23
    CXX_EXTENSIONS OFF
    VISIBILITY_INLINES_HIDDEN ON
    CXX_VISIBILITY_PRESET hidden
)

if (MSVC)
    target_compile_options(quotest PUBLIC /EHsc /W4
        /wd4100 /wd4127 /wd4242 /wd4244 /wd4245 /wd4267 /wd4365 /wd4456 /wd4459
        /wd4464 /wd4505 /wd4514 /wd4571 /wd4619 /wd4623 /wd4625 /wd4626 /wd4706
        /wd4710 /wd4774 /wd4820 /wd4946 /wd5026 /wd5027)
else()
    foreach (FLAG W Wall Wpedantic Wextra Wno-unused-parameter Werror=return-type)
        CHECK_CXX_COMPILER_FLAG("-${FLAG}" COMPILER_${FLAG}_SUPPORTED)
        if (COMPILER_${FLAG}_SUPPORTED AND
                NOT CMAKE_CXX_FLAGS MATCHES "(^| )-?${FLAG}($| )")
            target_compile_options(quotest PUBLIC -${FLAG})
        endif ()
    endforeach ()
endif()

option(${PROJECT_NAME}_INSTALL_TESTS "install quotest application" ON)
add_feature_info(InstallQuotest ${PROJECT_NAME}_INSTALL_TESTS
                "the library functional test suite")

if (${PROJECT_NAME}_INSTALL_TESTS)
    install(TARGETS quotest RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif ()
