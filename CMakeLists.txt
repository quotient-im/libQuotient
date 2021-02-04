cmake_minimum_required(VERSION 3.10)
if (POLICY CMP0092)
cmake_policy(SET CMP0092 NEW)
endif()

set(API_VERSION "0.7")
project(Quotient VERSION "${API_VERSION}.0" LANGUAGES CXX)

message(STATUS)
message(STATUS "Configuring ${PROJECT_NAME} ${PROJECT_VERSION} ==>")

include(FeatureSummary)

option(${PROJECT_NAME}_INSTALL_TESTS "install quotest (former qmc-example) application" ON)
add_feature_info(InstallQuotest ${PROJECT_NAME}_INSTALL_TESTS
                 "the library functional test suite")

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
    foreach (FLAG Wall W Wpedantic Wextra Werror=return-type Wno-unused-parameter
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

find_package(Qt5 5.9 REQUIRED Core Network Gui Multimedia Test)
get_filename_component(Qt5_Prefix "${Qt5_DIR}/../../../.." ABSOLUTE)
message(STATUS "Using Qt ${Qt5_VERSION} at ${Qt5_Prefix}")

if (${PROJECT_NAME}_ENABLE_E2EE)
    find_package(Olm 3.2.1 REQUIRED)
    set_package_properties(Olm PROPERTIES
        DESCRIPTION "Implementation of the Olm and Megolm cryptographic ratchets"
        URL "https://gitlab.matrix.org/matrix-org/olm"
        TYPE REQUIRED
    )
    if (Olm_FOUND)
        message(STATUS "Using libOlm ${Olm_VERSION} at ${Olm_DIR}")
    endif()

    find_package(OpenSSL REQUIRED)
    #find_package(OpenSSL 1.1.0 REQUIRED COMPONENTS SSL Crypto)
    set_package_properties(OpenSSL PROPERTIES
        DESCRIPTION "Open source SSL and TLS implementation and cryptographic library"
        URL "https://www.openssl.org/"
        TYPE REQUIRED
    )
    if (OpenSSL_FOUND)
        message(STATUS "Using OpenSSL ${OpenSSL_VERSION} at ${OpenSSL_DIR}")
    endif()
endif()

if (GTAD_PATH)
    get_filename_component(ABS_GTAD_PATH "${GTAD_PATH}" REALPATH)
endif ()
if (MATRIX_DOC_PATH)
    get_filename_component(ABS_API_DEF_PATH "${MATRIX_DOC_PATH}/api" REALPATH)
endif ()
if (ABS_GTAD_PATH AND ABS_API_DEF_PATH)
    message( STATUS "Using GTAD at ${ABS_GTAD_PATH}" )
    message( STATUS "Using API files at ${ABS_API_DEF_PATH}" )
    set(API_GENERATION_ENABLED 1)
    if (NOT CLANG_FORMAT)
        set(CLANG_FORMAT clang-format)
    endif()
    get_filename_component(ABS_CLANG_FORMAT "${CLANG_FORMAT}" PROGRAM)
    if (ABS_CLANG_FORMAT)
        set(API_FORMATTING_ENABLED 1)
        message( STATUS "clang-format is at ${ABS_CLANG_FORMAT}")
    else ()
        message( STATUS "${CLANG_FORMAT} is NOT FOUND; API files won't be reformatted")
    endif ()
endif()
add_feature_info(EnableApiCodeGeneration "${API_GENERATION_ENABLED}"
                 "build target update-api")
add_feature_info(EnableApiFormatting "${API_FORMATTING_ENABLED}"
                 "formatting of generated API files with clang-format")

message(STATUS)
feature_summary(WHAT ENABLED_FEATURES DISABLED_FEATURES
                FATAL_ON_MISSING_REQUIRED_PACKAGES)

# Set up source files
set(lib_SRCS
    lib/networkaccessmanager.cpp
    lib/connectiondata.cpp
    lib/connection.cpp
    lib/ssosession.cpp
    lib/logging.cpp
    lib/room.cpp
    lib/user.cpp
    lib/avatar.cpp
    lib/uri.cpp
    lib/uriresolver.cpp
    lib/syncdata.cpp
    lib/settings.cpp
    lib/networksettings.cpp
    lib/converters.cpp
    lib/util.cpp
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
    lib/events/roomkeyevent.cpp
    lib/events/stickerevent.cpp
    lib/jobs/requestdata.cpp
    lib/jobs/basejob.cpp
    lib/jobs/syncjob.cpp
    lib/jobs/mediathumbnailjob.cpp
    lib/jobs/downloadfilejob.cpp
    lib/crypto/qolmaccount.cpp
    lib/crypto/qolmsession.cpp
    lib/crypto/qolminboundsession.cpp
    lib/crypto/qolmoutboundsession.cpp
    lib/crypto/qolmutils.cpp
    lib/crypto/qolmutility.cpp
    lib/crypto/qolmerrors.cpp
    lib/crypto/qolmsession.cpp
    lib/crypto/qolmmessage.cpp
    lib/crypto/encryptionmanager.cpp
)

set(CSAPI_DIR csapi)
set(FULL_CSAPI_DIR lib/${CSAPI_DIR})
set(ASAPI_DEF_DIR application-service/definitions)
set(ISAPI_DEF_DIR identity/definitions)

if (MATRIX_DOC_PATH AND GTAD_PATH)
    if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.12.0")
        # We use globbing with CONFIGURE_DEPENDS to produce two file lists:
        # one of all API files for clang-format and another of just .cpp
        # files to supply for library source files. Since we expect these
        # file lists to only change due to GTAD invocation, we only use
        # CONFIGURE_DEPENDS when pre-requisites to update API are met.
        # Read comments next to each file(GLOB_RECURSE) for caveats.
        set(add_CONFIGURE_DEPENDS "CONFIGURE_DEPENDS")
    endif()
    set(FULL_CSAPI_SRC_DIR ${ABS_API_DEF_PATH}/client-server)
    file(GLOB_RECURSE API_DEFS RELATIVE ${PROJECT_SOURCE_DIR}
        ${FULL_CSAPI_SRC_DIR}/*.yaml
        ${ABS_API_DEF_PATH}/${ASAPI_DEF_DIR}/*.yaml
        ${ABS_API_DEF_PATH}/${ISAPI_DEF_DIR}/*.yaml
    )
    add_custom_target(generate-unformatted-api
        ${ABS_GTAD_PATH} --config ../gtad/gtad.yaml --out ${CSAPI_DIR}
            ${FULL_CSAPI_SRC_DIR}
            old_sync.yaml- room_initial_sync.yaml- # deprecated
            key_backup.yaml- # immature and buggy in terms of API definition
            sync.yaml- # we have a better handcrafted implementation
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/lib
        SOURCES gtad/gtad.yaml
                gtad/data.h.mustache
                gtad/operation.h.mustache
                gtad/operation.cpp.mustache
                ${API_DEFS}
        VERBATIM
    )
    add_custom_target(update-api DEPENDS generate-unformatted-api)
    if (ABS_CLANG_FORMAT)
        set(CLANG_FORMAT_ARGS -i -sort-includes ${CLANG_FORMAT_ARGS})
        # FIXME: the list of files should be produced after GTAD has run.
        # For now it's produced at CMake invocation. If file() hasn't found
        # any files at that moment, clang-format will be called with an empty
        # list of files and choke. Taking outfiles.txt from GTAD could be
        # an option but this, too, must be done during the build stage, and
        # there's no crossplatform way to use the contents of a file as
        # input for a build target command.
        file(GLOB_RECURSE api_ALL_SRCS ${add_CONFIGURE_DEPENDS}
             ${FULL_CSAPI_DIR}/*.*
             lib/${ASAPI_DEF_DIR}/*.*
             lib/${ISAPI_DEF_DIR}/*.*)
        if (api_ALL_SRCS)
            add_custom_target(format-api
                ${ABS_CLANG_FORMAT} ${CLANG_FORMAT_ARGS} ${api_ALL_SRCS}
                DEPENDS generate-unformatted-api
                VERBATIM)
            add_dependencies(update-api format-api)
        endif()
    endif()
endif()

# Make no mistake: CMake cannot run gtad first and then populate the list of
# resulting api_SRCS files. In other words, placing the below statement after
# the block above will not lead to CMake magically reconfiguring the build
# after building the update-api target. CONFIGURE_DEPENDS somewhat helps,
# at least forcing the build system to reevaluate the glob before building
# the next target. Otherwise, you have to watch out if gtad has created
# new files and if it has, re-run cmake.
file(GLOB_RECURSE api_SRCS ${add_CONFIGURE_DEPENDS} ${FULL_CSAPI_DIR}/*.cpp)

add_library(${PROJECT_NAME} ${lib_SRCS} ${api_SRCS})
target_compile_definitions(${PROJECT_NAME} PRIVATE QT_NO_JAVA_STYLE_ITERATORS QT_NO_URL_CAST_FROM_STRING QT_NO_CAST_TO_ASCII)
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.16.0"
        AND NOT CMAKE_CXX_COMPILER_ID STREQUAL GNU) # https://bugzilla.redhat.com/show_bug.cgi?id=1721553
    target_precompile_headers(${PROJECT_NAME} PRIVATE lib/converters.h)
endif ()
if (${PROJECT_NAME}_ENABLE_E2EE)
    target_compile_definitions(${PROJECT_NAME} PUBLIC ${PROJECT_NAME}_E2EE_ENABLED)
endif()
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
    $<INSTALL_INTERFACE:${${PROJECT_NAME}_INSTALL_INCLUDEDIR}>
)
if (${PROJECT_NAME}_ENABLE_E2EE)
    target_link_libraries(${PROJECT_NAME} Olm::Olm
        OpenSSL::Crypto
        OpenSSL::SSL)
    set(FIND_DEPS "find_dependency(Olm)
    find_dependency(OpenSSL)") # For QuotientConfig.cmake.in
endif()
target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Network Qt5::Gui Qt5::Multimedia)

if (BUILD_TESTING)
    enable_testing()
    add_subdirectory(quotest)
    add_subdirectory(autotests)
endif()

configure_file(${PROJECT_NAME}.pc.in ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc @ONLY NEWLINE_STYLE UNIX)

# Installation

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

if (${PROJECT_NAME}_INSTALL_TESTS)
    install(TARGETS ${TEST_BINARY} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif ()

if (UNIX AND NOT APPLE)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endif()

message(STATUS "<== End of libQuotient configuration")
