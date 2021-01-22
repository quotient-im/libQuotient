# Officially CMake 3.16+ is needed but LGTM.com still sits on eoan that only
# has CMake 3.13
cmake_minimum_required(VERSION 3.13)
if (POLICY CMP0092)
cmake_policy(SET CMP0092 NEW)
endif()

set(API_VERSION "0.7")
project(Quotient VERSION "${API_VERSION}.0" LANGUAGES CXX)

message(STATUS)
message(STATUS "Configuring ${PROJECT_NAME} ${PROJECT_VERSION} ==>")

include(FeatureSummary)
include(CTest)

# https://github.com/quotient-im/libQuotient/issues/369
option(${PROJECT_NAME}_ENABLE_E2EE "end-to-end encryption (E2EE) support" OFF)
add_feature_info(EnableE2EE ${PROJECT_NAME}_ENABLE_E2EE
                 "end-to-end encryption (WORK IN PROGRESS)")

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Debug' as none was specified")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build" FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()
if (CMAKE_BUILD_TYPE)
    message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
endif(CMAKE_BUILD_TYPE)

message(STATUS "Using compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}" )
include(CheckCXXCompilerFlag)
if (MSVC)
    add_compile_options(/EHsc /W4
        /wd4100 /wd4127 /wd4242 /wd4244 /wd4245 /wd4267 /wd4365 /wd4456 /wd4459
        /wd4464 /wd4505 /wd4514 /wd4571 /wd4619 /wd4623 /wd4625 /wd4626 /wd4706
        /wd4710 /wd4774 /wd4820 /wd4946 /wd5026 /wd5027)
else()
    foreach (FLAG Wall Wpedantic Wextra Werror=return-type Wno-unused-parameter
            Wno-gnu-zero-variadic-macro-arguments fvisibility-inlines-hidden)
        CHECK_CXX_COMPILER_FLAG("-${FLAG}" COMPILER_${FLAG}_SUPPORTED)
        if ( COMPILER_${FLAG}_SUPPORTED AND
                NOT CMAKE_CXX_FLAGS MATCHES "(^| )-?${FLAG}($| )")
            add_compile_options(-${FLAG})
        endif ()
    endforeach ()
endif()

if (WIN32)
    if (NOT CMAKE_INSTALL_LIBDIR)
        set(CMAKE_INSTALL_LIBDIR ".")
    endif ()

    if (NOT CMAKE_INSTALL_BINDIR)
        set(CMAKE_INSTALL_BINDIR ".")
    endif ()

    if (NOT CMAKE_INSTALL_INCLUDEDIR)
        set(CMAKE_INSTALL_INCLUDEDIR "include")
    endif ()
else()
    include(GNUInstallDirs)
    set(INCLUDEDIR_INIT ${PROJECT_NAME})
endif(WIN32)
set(${PROJECT_NAME}_INSTALL_INCLUDEDIR
        "${CMAKE_INSTALL_INCLUDEDIR}/${INCLUDEDIR_INIT}" CACHE PATH
        "directory to install ${PROJECT_NAME} include files to")
message(STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Header files will be installed to ${CMAKE_INSTALL_PREFIX}/${${PROJECT_NAME}_INSTALL_INCLUDEDIR}")

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

option(BUILD_WITH_QT6 "Build Quotient with Qt 6 (EXPERIMENTAL)" OFF)

if (BUILD_WITH_QT6)
    set(QtMinVersion "6.0")
else()
    set(QtMinVersion "5.12")
    set(QtExtraModules "Multimedia") # See #483
endif()
string(REGEX REPLACE "^(.).*" "Qt\\1" Qt ${QtMinVersion}) # makes "Qt5" or "Qt6"
find_package(${Qt} ${QtMinVersion} REQUIRED Core Network Gui Test ${QtExtraModules})
get_filename_component(Qt_Prefix "${${Qt}_DIR}/../../../.." ABSOLUTE)
message(STATUS "Using Qt ${${Qt}_VERSION} at ${Qt_Prefix}")

if (${PROJECT_NAME}_ENABLE_E2EE)
    if ((NOT DEFINED USE_INTREE_LIBQOLM OR USE_INTREE_LIBQOLM)
            AND EXISTS ${PROJECT_SOURCE_DIR}/3rdparty/libQtOlm/lib/utils.h)
        add_subdirectory(3rdparty/libQtOlm)
        include_directories(3rdparty/libQtOlm)
        if (NOT DEFINED USE_INTREE_LIBQOLM)
            set (USE_INTREE_LIBQOLM 1)
        endif ()
    endif ()
    if (USE_INTREE_LIBQOLM)
        message( STATUS "Using in-tree libQtOlm")
        find_package(Git QUIET)
        if (GIT_FOUND)
            execute_process(COMMAND
                "${GIT_EXECUTABLE}" rev-parse -q HEAD
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/3rdparty/libQtOlm
                OUTPUT_VARIABLE QTOLM_GIT_SHA1
                OUTPUT_STRIP_TRAILING_WHITESPACE)
            message( STATUS "  Library git SHA1: ${QTOLM_GIT_SHA1}")
        endif (GIT_FOUND)
    else ()
        set(SAVED_CMAKE_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR})
        set(CMAKE_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR})
        find_package(QtOlm 3.0.1 REQUIRED)
        set_package_properties(QtOlm PROPERTIES
            DESCRIPTION "QtOlm is a Qt wrapper around libOlm"
            PURPOSE "libQtOlm is required to support end-to-end encryption. See also BUILDING.md"
            URL "https://gitlab.com/b0/libqtolm"
            )
        if (QtOlm_FOUND)
            message(STATUS "Using libQtOlm ${QtOlm_VERSION} at ${QtOlm_DIR}")
        endif()
    endif ()
endif ()

# Set up source files
list(APPEND lib_SRCS
    lib/quotient_common.h
    lib/function_traits.h lib/function_traits.cpp
    lib/networkaccessmanager.h lib/networkaccessmanager.cpp
    lib/connectiondata.h lib/connectiondata.cpp
    lib/connection.h lib/connection.cpp
    lib/ssosession.h lib/ssosession.cpp
    lib/logging.h lib/logging.cpp
    lib/room.h lib/room.cpp
    lib/user.h lib/user.cpp
    lib/avatar.h lib/avatar.cpp
    lib/uri.h lib/uri.cpp
    lib/uriresolver.h lib/uriresolver.cpp
    lib/eventstats.h lib/eventstats.cpp
    lib/syncdata.h lib/syncdata.cpp
    lib/settings.h lib/settings.cpp
    lib/networksettings.h lib/networksettings.cpp
    lib/converters.h lib/converters.cpp
    lib/util.h lib/util.cpp
    lib/encryptionmanager.h lib/encryptionmanager.cpp
    lib/eventitem.h lib/eventitem.cpp
    lib/accountregistry.h lib/accountregistry.cpp
    lib/mxcreply.h lib/mxcreply.cpp
    lib/events/event.h lib/events/event.cpp
    lib/events/roomevent.h lib/events/roomevent.cpp
    lib/events/stateevent.h lib/events/stateevent.cpp
    lib/events/simplestateevents.h
    lib/events/eventcontent.h lib/events/eventcontent.cpp
    lib/events/roomcreateevent.h lib/events/roomcreateevent.cpp
    lib/events/roomtombstoneevent.h lib/events/roomtombstoneevent.cpp
    lib/events/roommessageevent.h lib/events/roommessageevent.cpp
    lib/events/roommemberevent.h lib/events/roommemberevent.cpp
    lib/events/roomavatarevent.h
    lib/events/roompowerlevelsevent.h lib/events/roompowerlevelsevent.cpp
    lib/events/typingevent.h lib/events/typingevent.cpp
    lib/events/accountdataevents.h
    lib/events/receiptevent.h lib/events/receiptevent.cpp
    lib/events/reactionevent.h lib/events/reactionevent.cpp
    lib/events/callanswerevent.h lib/events/callanswerevent.cpp
    lib/events/callcandidatesevent.h lib/events/callcandidatesevent.cpp
    lib/events/callhangupevent.h lib/events/callhangupevent.cpp
    lib/events/callinviteevent.h lib/events/callinviteevent.cpp
    lib/events/directchatevent.h lib/events/directchatevent.cpp
    lib/events/encryptionevent.h lib/events/encryptionevent.cpp
    lib/events/encryptedevent.h lib/events/encryptedevent.cpp
    lib/events/roomkeyevent.h lib/events/roomkeyevent.cpp
    lib/events/stickerevent.h lib/events/stickerevent.cpp
    lib/jobs/requestdata.h lib/jobs/requestdata.cpp
    lib/jobs/basejob.h lib/jobs/basejob.cpp
    lib/jobs/syncjob.h lib/jobs/syncjob.cpp
    lib/jobs/mediathumbnailjob.h lib/jobs/mediathumbnailjob.cpp
    lib/jobs/downloadfilejob.h lib/jobs/downloadfilejob.cpp
    lib/olm/qolmaccount.cpp
)

# Configure API files generation

set(CSAPI_DIR csapi)
set(FULL_CSAPI_DIR lib/${CSAPI_DIR})
set(ASAPI_DEF_DIR application-service/definitions)
set(ISAPI_DEF_DIR identity/definitions)

set(API_GENERATION_ENABLED 0)
if (GTAD_PATH AND MATRIX_DOC_PATH)
    # REALPATH resolves ~ (home directory) while PROGRAM doesn't
    get_filename_component(ABS_GTAD_PATH "${GTAD_PATH}" REALPATH)
    get_filename_component(ABS_GTAD_PATH "${ABS_GTAD_PATH}" PROGRAM PROGRAM_ARGS GTAD_ARGS)
    if (EXISTS ${ABS_GTAD_PATH})
        get_filename_component(ABS_API_DEF_PATH "${MATRIX_DOC_PATH}/data/api" REALPATH)
        if (NOT IS_DIRECTORY ${ABS_API_DEF_PATH})
            # Check the old place of API files
            get_filename_component(ABS_API_DEF_PATH "${MATRIX_DOC_PATH}/api" REALPATH)
        endif ()
        if (IS_DIRECTORY ${ABS_API_DEF_PATH})
            set(API_GENERATION_ENABLED 1)
        else ()
            message( WARNING "${MATRIX_DOC_PATH} doesn't seem to point to a valid matrix-doc repo; disabling API stubs generation")
        endif ()
    else (EXISTS ${ABS_GTAD_PATH})
        message( WARNING "${GTAD_PATH} is not executable; disabling API stubs generation")
    endif ()
endif ()
if (API_GENERATION_ENABLED)
    message( STATUS "Using GTAD at ${ABS_GTAD_PATH}" )
    message( STATUS "Found API files at ${ABS_API_DEF_PATH}" )
    if (NOT CLANG_FORMAT)
        set(CLANG_FORMAT clang-format)
    endif()
    get_filename_component(ABS_CLANG_FORMAT "${CLANG_FORMAT}" PROGRAM PROGRAM_ARGS CLANG_FORMAT_ARGS)
    if (NOT ABS_CLANG_FORMAT)
        message( WARNING "${CLANG_FORMAT} is NOT FOUND; API files won't be formatted")
    endif ()

    set(FULL_CSAPI_SRC_DIR ${ABS_API_DEF_PATH}/client-server)
    file(GLOB_RECURSE API_DEFS RELATIVE ${PROJECT_SOURCE_DIR}
        ${FULL_CSAPI_SRC_DIR}/*.yaml
        ${ABS_API_DEF_PATH}/${ASAPI_DEF_DIR}/*.yaml
        ${ABS_API_DEF_PATH}/${ISAPI_DEF_DIR}/*.yaml
    )
    add_custom_target(update-api
        ${ABS_GTAD_PATH} --config ../gtad/gtad.yaml --out ${CSAPI_DIR}
            ${FULL_CSAPI_SRC_DIR}
            old_sync.yaml- room_initial_sync.yaml- # deprecated
            key_backup.yaml- # immature and buggy in terms of API definition
            sync.yaml- # we have a better handcrafted implementation
            ${GTAD_ARGS}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/lib
        SOURCES gtad/gtad.yaml
                gtad/data.h.mustache
                gtad/operation.h.mustache
                gtad/operation.cpp.mustache
                ${API_DEFS}
        VERBATIM
    )
endif()
add_feature_info(EnableApiCodeGeneration "${API_GENERATION_ENABLED}"
                 "build target update-api")

# Produce the list of all Matrix API files for building the library. When this
# list changes (normally after calling GTAD), CONFIGURE_DEPENDS will force
# the build system to call CMake again. Checking for the glob change slows down
# each build (even if the target does not involve API generation). It would be
# ideal if GTAD could compare the initial (saved somewhere) and the generated
# file list itself and write down to some .cmake file if those are different,
# which would trigger the reconfiguration specifically before the next build.
# For now CONFIGURE_DEPENDS is the best approximation of that.
file(GLOB_RECURSE api_ALL_SRCS CONFIGURE_DEPENDS
     ${FULL_CSAPI_DIR}/*.* lib/${ASAPI_DEF_DIR}/*.* lib/${ISAPI_DEF_DIR}/*.*)

add_library(${PROJECT_NAME} ${lib_SRCS} ${api_ALL_SRCS})
target_compile_definitions(${PROJECT_NAME} PRIVATE QT_NO_JAVA_STYLE_ITERATORS QT_NO_URL_CAST_FROM_STRING QT_NO_CAST_TO_ASCII)

target_compile_definitions(${PROJECT_NAME} PUBLIC ${PROJECT_NAME}_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    ${PROJECT_NAME}_VERSION_MINOR=${PROJECT_VERSION_MINOR} ${PROJECT_NAME}_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    ${PROJECT_NAME}_VERSION_STRING=\"${PROJECT_VERSION}\")
if (${PROJECT_NAME}_ENABLE_E2EE)
    target_compile_definitions(${PROJECT_NAME} PUBLIC ${PROJECT_NAME}_E2EE_ENABLED)
endif()
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_EXTENSIONS OFF
    VERSION "${PROJECT_VERSION}"
    SOVERSION ${API_VERSION}
    INTERFACE_${PROJECT_NAME}_MAJOR_VERSION ${API_VERSION}
)
set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY
             COMPATIBLE_INTERFACE_STRING ${PROJECT_NAME}_MAJOR_VERSION)

# C++17 required, C++20 desired (see above)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

# TODO: Bump the CMake requirement and drop the version check here once
# LGTM upgrades
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.16.0"
        AND NOT CMAKE_CXX_COMPILER_ID STREQUAL GNU) # https://bugzilla.redhat.com/show_bug.cgi?id=1721553
    target_precompile_headers(${PROJECT_NAME} PRIVATE lib/converters.h)
endif ()

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib>
    $<INSTALL_INTERFACE:${${PROJECT_NAME}_INSTALL_INCLUDEDIR}>
)
if (${PROJECT_NAME}_ENABLE_E2EE)
    target_link_libraries(${PROJECT_NAME} Olm::Olm QtOlm)
    set(FIND_DEPS "find_dependency(QtOlm)") # For QuotientConfig.cmake.in
endif()
target_link_libraries(${PROJECT_NAME} ${Qt}::Core ${Qt}::Network ${Qt}::Gui)
if (Qt STREQUAL Qt5) # See #483
    target_link_libraries(${PROJECT_NAME} ${Qt}::Multimedia)
endif()

configure_file(${PROJECT_NAME}.pc.in ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc @ONLY NEWLINE_STYLE UNIX)

# Configure testing

if (BUILD_TESTING)
    enable_testing()
    add_subdirectory(quotest)
    add_subdirectory(autotests)
endif()

# Configure installation

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES DESTINATION ${${PROJECT_NAME}_INSTALL_INCLUDEDIR}
)
install(DIRECTORY lib/ DESTINATION ${${PROJECT_NAME}_INSTALL_INCLUDEDIR}
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
configure_file(cmake/${PROJECT_NAME}Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}Config.cmake"
    @ONLY
)

set(ConfigFilesLocation "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
install(EXPORT ${PROJECT_NAME}Targets
        FILE ${PROJECT_NAME}Targets.cmake DESTINATION ${ConfigFilesLocation})

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION ${ConfigFilesLocation}
)
install(EXPORT_ANDROID_MK ${PROJECT_NAME}Targets DESTINATION ${CMAKE_INSTALL_DATADIR}/ndk-modules)

if (WIN32)
    install(FILES mime/packages/freedesktop.org.xml DESTINATION mime/packages)
endif (WIN32)

if (UNIX AND NOT APPLE)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endif()

message(STATUS)
feature_summary(WHAT ENABLED_FEATURES DISABLED_FEATURES
                FATAL_ON_MISSING_REQUIRED_PACKAGES)

message(STATUS "<== End of libQuotient configuration")
