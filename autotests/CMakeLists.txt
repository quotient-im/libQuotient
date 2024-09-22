# SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
#
# SPDX-License-Identifier: BSD-3-Clause

include(CMakeParseArguments)

add_custom_target(autotests ALL)

function(QUOTIENT_ADD_TEST)
    cmake_parse_arguments(ARG "" "NAME" "" ${ARGN})
    add_executable(${ARG_NAME} ${ARG_NAME}.cpp testutils.h testutils.cpp)
    target_link_libraries(${ARG_NAME} ${Qt}::Core ${Qt}::Test ${QUOTIENT_LIB_NAME})
    add_test(NAME ${ARG_NAME} COMMAND ${ARG_NAME})
    add_dependencies(autotests ${ARG_NAME})
endfunction()

quotient_add_test(NAME callcandidateseventtest)
quotient_add_test(NAME utiltests)
