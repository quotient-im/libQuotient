# SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
#
# SPDX-License-Identifier: BSD-3-Clause

set(quotest_SRCS quotest.cpp)

find_package(${Qt} COMPONENTS Concurrent)
add_executable(quotest ${quotest_SRCS})
target_link_libraries(quotest PRIVATE ${Qt}::Core ${Qt}::Test ${Qt}::Concurrent ${PROJECT_NAME})

set_target_properties(quotest PROPERTIES
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
