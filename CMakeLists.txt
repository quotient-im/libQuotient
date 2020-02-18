cmake_minimum_required(VERSION 3.10)
if (POLICY CMP0092)
cmake_policy(SET CMP0092 NEW)
endif()

set(API_VERSION "0.6")
project(Quotient VERSION "${API_VERSION}.0" LANGUAGES CXX)

option(${PROJECT_NAME}_INSTALL_TESTS "install quotest (former qmc-example) application" ON)

include(CheckCXXCompilerFlag)
if (NOT WIN32)
    include(GNUInstallDirs)
endif(NOT WIN32)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Debug' as none was specified")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build" FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()

if (NOT CMAKE_INSTALL_LIBDIR)
    set(CMAKE_INSTALL_LIBDIR ".")
endif()

if (NOT CMAKE_INSTALL_BINDIR)
    set(CMAKE_INSTALL_BINDIR ".")
endif()

if (NOT CMAKE_INSTALL_INCLUDEDIR)
    set(CMAKE_INSTALL_INCLUDEDIR "include")
endif()

if (MSVC)
    add_compile_options(/EHsc /W4
        /wd4100 /wd4127 /wd4242 /wd4244 /wd4245 /wd4267 /wd4365 /wd4456 /wd4459
        /wd4464 /wd4505 /wd4514 /wd4571 /wd4619 /wd4623 /wd4625 /wd4626 /wd4706
        /wd4710 /wd4774 /wd4820 /wd4946 /wd5026 /wd5027)
else()
    foreach (FLAG all "" pedantic extra error=return-type no-unused-parameter
                  no-gnu-zero-variadic-macro-arguments)
        CHECK_CXX_COMPILER_FLAG("-W${FLAG}" WARN_${FLAG}_SUPPORTED)
        if ( WARN_${FLAG}_SUPPORTED AND NOT CMAKE_CXX_FLAGS MATCHES "(^| )-W?${FLAG}($| )")
            add_compile_options(-W${FLAG})
        endif ()
    endforeach ()
endif()

find_package(Qt5 5.9 REQUIRED Network Gui Multimedia Test)
get_filename_component(Qt5_Prefix "${Qt5_DIR}/../../../.." ABSOLUTE)

if ((NOT DEFINED USE_INTREE_LIBQOLM OR USE_INTREE_LIBQOLM)
        AND EXISTS ${PROJECT_SOURCE_DIR}/3rdparty/libQtOlm/lib/utils.h)
    add_subdirectory(3rdparty/libQtOlm EXCLUDE_FROM_ALL)
    include_directories(3rdparty/libQtOlm)
    if (NOT DEFINED USE_INTREE_LIBQOLM)
        set (USE_INTREE_LIBQOLM 1)
    endif ()
endif ()
if (NOT USE_INTREE_LIBQOLM)
    find_package(QtOlm 0.1.0 REQUIRED)
    if (NOT QtOlm_FOUND)
        message( WARNING "libQtOlm not found; configuration will most likely fail.")
        message( WARNING "Make sure you have installed libQtOlm development files")
        message( WARNING "as a package or checked out the library sources in lib/.")
        message( WARNING "See also BUILDING.md")
    endif ()
endif ()

if (GTAD_PATH)
    get_filename_component(ABS_GTAD_PATH "${GTAD_PATH}" REALPATH)
endif ()
if (MATRIX_DOC_PATH)
    get_filename_component(ABS_API_DEF_PATH "${MATRIX_DOC_PATH}/api" REALPATH)
endif ()
if (ABS_GTAD_PATH AND ABS_API_DEF_PATH)
    if (NOT CLANG_FORMAT)
        set(CLANG_FORMAT clang-format)
    endif()
    get_filename_component(ABS_CLANG_FORMAT "${CLANG_FORMAT}" PROGRAM)
endif()

message( STATUS )
message( STATUS "=============================================================================" )
message( STATUS "                       ${PROJECT_NAME} Build Information" )
message( STATUS "=============================================================================" )
message( STATUS "Version: ${PROJECT_VERSION}, API version: ${API_VERSION}")
if (CMAKE_BUILD_TYPE)
    message( STATUS "Build type: ${CMAKE_BUILD_TYPE}")
endif(CMAKE_BUILD_TYPE)
message( STATUS "Using compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}" )
message( STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}" )
message( STATUS "Using Qt ${Qt5_VERSION} at ${Qt5_Prefix}" )
if (ABS_API_DEF_PATH AND ABS_GTAD_PATH)
    message( STATUS "Generating API stubs enabled (use --target update-api)" )
    message( STATUS "  Using GTAD at ${ABS_GTAD_PATH}" )
    message( STATUS "  Using API files at ${ABS_API_DEF_PATH}" )
    if (ABS_CLANG_FORMAT)
        message( STATUS "clang-format is at ${ABS_CLANG_FORMAT}")
    else ()
        message( STATUS "${CLANG_FORMAT} is NOT FOUND; API files won't be reformatted")
    endif ()
endif ()
find_package(Git)
if (USE_INTREE_LIBQOLM)
    message( STATUS "Using in-tree libQtOlm")
    if (GIT_FOUND)
        execute_process(COMMAND
            "${GIT_EXECUTABLE}" rev-parse -q HEAD
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/3rdparty/libQtOlm
            OUTPUT_VARIABLE QTOLM_GIT_SHA1
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        message( STATUS "  Library git SHA1: ${QTOLM_GIT_SHA1}")
    endif (GIT_FOUND)
else ()
    message( STATUS "Using libQtOlm ${QtOlm_VERSION} at ${QtOlm_DIR}")
endif ()
message( STATUS "=============================================================================" )
message( STATUS )

# Set up source files
set(lib_SRCS
    lib/networkaccessmanager.cpp
    lib/connectiondata.cpp
    lib/connection.cpp
    lib/logging.cpp
    lib/room.cpp
    lib/user.cpp
    lib/avatar.cpp
    lib/syncdata.cpp
    lib/settings.cpp
    lib/networksettings.cpp
    lib/converters.cpp
    lib/util.cpp
    lib/encryptionmanager.cpp
    lib/eventitem.cpp
    lib/events/event.cpp
    lib/events/roomevent.cpp
    lib/events/stateevent.cpp
    lib/events/eventcontent.cpp
    lib/events/roomcreateevent.cpp
    lib/events/roomtombstoneevent.cpp
    lib/events/roommessageevent.cpp
    lib/events/roommemberevent.cpp
    lib/events/roompowerlevelsevent.cpp
    lib/events/typingevent.cpp
    lib/events/receiptevent.cpp
    lib/events/reactionevent.cpp
    lib/events/callanswerevent.cpp
    lib/events/callcandidatesevent.cpp
    lib/events/callhangupevent.cpp
    lib/events/callinviteevent.cpp
    lib/events/directchatevent.cpp
    lib/events/encryptionevent.cpp
    lib/events/encryptedevent.cpp
    lib/jobs/requestdata.cpp
    lib/jobs/basejob.cpp
    lib/jobs/syncjob.cpp
    lib/jobs/mediathumbnailjob.cpp
    lib/jobs/downloadfilejob.cpp
)

set(CSAPI_DIR csapi)
set(ASAPI_DEF_DIR application-service/definitions)
set(ISAPI_DEF_DIR identity/definitions)

foreach (D ${CSAPI_DIR} ${CSAPI_DIR}/definitions
         ${CSAPI_DIR}/definitions/wellknown ${ASAPI_DEF_DIR} ${ISAPI_DEF_DIR})
    aux_source_directory(lib/${D} api_SRCS)
endforeach()

# Make no mistake: CMake cannot run gtad first and then populate the list of
# resulting api_SRCS files. In other words, placing the above foreach after
# the custom targets definition won't bring the desired result:
# CMake will execute it at cmake invocation and gtad will only run later
# when building the update-api target. If you see that gtad has created
# new files you have to re-run cmake.
# TODO: check `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)` (from CMake 3.14)
if (MATRIX_DOC_PATH AND GTAD_PATH)
    set(FULL_CSAPI_DIR lib/${CSAPI_DIR})
    set(FULL_CSAPI_SRC_DIR ${ABS_API_DEF_PATH}/client-server)
    file(GLOB_RECURSE API_DEFS RELATIVE ${PROJECT_SOURCE_DIR}
        ${FULL_CSAPI_SRC_DIR}/*.yaml
        ${ABS_API_DEF_PATH}/${ASAPI_DEF_DIR}/*.yaml
        ${ABS_API_DEF_PATH}/${ISAPI_DEF_DIR}/*.yaml
    )
    add_custom_target(update-api
        ${ABS_GTAD_PATH} --config ${CSAPI_DIR}/gtad.yaml --out ${CSAPI_DIR}
            ${FULL_CSAPI_SRC_DIR}
            old_sync.yaml- room_initial_sync.yaml- # deprecated
            search.yaml- # current GTAD is limited in handling move-only data
            sync.yaml- # we have a better handcrafted implementation
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/lib
        SOURCES ${FULL_CSAPI_DIR}/gtad.yaml
                ${FULL_CSAPI_DIR}/{{base}}.h.mustache
                ${FULL_CSAPI_DIR}/{{base}}.cpp.mustache
                ${API_DEFS}
        VERBATIM
    )
    if (ABS_CLANG_FORMAT)
        # TODO: list(TRANSFORM) is available from CMake 3.12
        foreach (S ${api_SRCS})
            string (REGEX REPLACE ".cpp$" ".h" H ${S})
            list(APPEND api_HDRS ${H})
        endforeach()
        set(CLANG_FORMAT_ARGS -i -sort-includes ${CLANG_FORMAT_ARGS})
        add_custom_command(TARGET update-api POST_BUILD
            COMMAND ${ABS_CLANG_FORMAT} ${CLANG_FORMAT_ARGS} ${api_SRCS}
            COMMAND ${ABS_CLANG_FORMAT} ${CLANG_FORMAT_ARGS} ${api_HDRS}
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            VERBATIM
            COMMENT Formatting files
        )
    endif()
endif()

set(tests_SRCS tests/quotest.cpp)

add_library(${PROJECT_NAME} ${lib_SRCS} ${api_SRCS})
set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION "${PROJECT_VERSION}"
    SOVERSION ${API_VERSION}
    INTERFACE_${PROJECT_NAME}_MAJOR_VERSION ${API_VERSION}
)
set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY
             COMPATIBLE_INTERFACE_STRING ${PROJECT_NAME}_MAJOR_VERSION)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(${PROJECT_NAME} QtOlm Qt5::Core Qt5::Network Qt5::Gui Qt5::Multimedia)

set(TEST_BINARY quotest)
add_executable(${TEST_BINARY} ${tests_SRCS})
target_link_libraries(${TEST_BINARY} Qt5::Core Qt5::Test ${PROJECT_NAME})

configure_file(${PROJECT_NAME}.pc.in ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc @ONLY NEWLINE_STYLE UNIX)

# Installation

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY lib/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h")

include(CMakePackageConfigHelpers)
# NB: SameMajorVersion doesn't really work yet, as we're within 0.x trail.
# Maybe consider jumping the gun and releasing 1.0, as semver advises?
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}ConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
)

export(PACKAGE ${PROJECT_NAME})
export(EXPORT ${PROJECT_NAME}Targets
       FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}Targets.cmake")
configure_file(cmake/${PROJECT_NAME}Config.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}Config.cmake"
    COPYONLY
)

set(ConfigFilesLocation "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
install(EXPORT ${PROJECT_NAME}Targets
        FILE ${PROJECT_NAME}Targets.cmake DESTINATION ${ConfigFilesLocation})

install(FILES cmake/${PROJECT_NAME}Config.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION ${ConfigFilesLocation}
)
install(EXPORT_ANDROID_MK ${PROJECT_NAME}Targets DESTINATION share/ndk-modules)

if (WIN32)
    install(FILES mime/packages/freedesktop.org.xml DESTINATION mime/packages)
endif (WIN32)

if (${PROJECT_NAME}_INSTALL_TESTS)
    install(TARGETS ${TEST_BINARY} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif ()

if (UNIX AND NOT APPLE)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endif()
