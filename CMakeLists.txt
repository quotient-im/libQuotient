cmake_minimum_required(VERSION 3.1)

project(qmatrixclient CXX)

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

set(CMAKE_CXX_STANDARD 14)

foreach (FLAG all "" pedantic extra error=return-type no-unused-parameter no-gnu-zero-variadic-macro-arguments)
    CHECK_CXX_COMPILER_FLAG("-W${FLAG}" WARN_${FLAG}_SUPPORTED)
    if ( WARN_${FLAG}_SUPPORTED AND NOT CMAKE_CXX_FLAGS MATCHES "(^| )-W?${FLAG}($| )")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W${FLAG}")
    endif ()
endforeach ()

find_package(Qt5 5.4.1 REQUIRED Network Gui)
get_filename_component(Qt5_Prefix "${Qt5_DIR}/../../../.." ABSOLUTE)

if (GTAD_PATH)
    get_filename_component(ABS_GTAD_PATH "${GTAD_PATH}" ABSOLUTE)
endif ()
if (MATRIX_DOC_PATH)
    get_filename_component(ABS_API_DEF_PATH "${MATRIX_DOC_PATH}/api" ABSOLUTE)
endif ()

message( STATUS )
message( STATUS "=============================================================================" )
message( STATUS "                       libqmatrixclient Build Information" )
message( STATUS "=============================================================================" )
if (CMAKE_BUILD_TYPE)
    message( STATUS "Build type: ${CMAKE_BUILD_TYPE}")
endif(CMAKE_BUILD_TYPE)
message( STATUS "Using compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}" )
message( STATUS "Using Qt ${Qt5_VERSION} at ${Qt5_Prefix}" )
if (MATRIX_DOC_PATH AND GTAD_PATH)
    message( STATUS "Generating API stubs enabled" )
    message( STATUS "  Using GTAD at ${ABS_GTAD_PATH}" )
    message( STATUS "  Using API files at ${ABS_API_DEF_PATH}" )
endif ()
message( STATUS "=============================================================================" )
message( STATUS )

# Set up source files
set(libqmatrixclient_SRCS
    lib/networkaccessmanager.cpp
    lib/connectiondata.cpp
    lib/connection.cpp
    lib/logging.cpp
    lib/room.cpp
    lib/user.cpp
    lib/avatar.cpp
    lib/settings.cpp
    lib/networksettings.cpp
    lib/converters.cpp
    lib/util.cpp
    lib/eventitem.cpp
    lib/events/event.cpp
    lib/events/roomevent.cpp
    lib/events/stateevent.cpp
    lib/events/eventcontent.cpp
    lib/events/roommessageevent.cpp
    lib/events/roommemberevent.cpp
    lib/events/typingevent.cpp
    lib/events/receiptevent.cpp
    lib/events/callanswerevent.cpp
    lib/events/callcandidatesevent.cpp
    lib/events/callhangupevent.cpp
    lib/events/callinviteevent.cpp
    lib/events/directchatevent.cpp
    lib/jobs/requestdata.cpp
    lib/jobs/basejob.cpp
    lib/jobs/syncjob.cpp
    lib/jobs/mediathumbnailjob.cpp
    lib/jobs/downloadfilejob.cpp
)

set(CSAPI_DIR csapi)
set(FULL_CSAPI_DIR lib/${CSAPI_DIR})
set(FULL_CSAPI_SRC_DIR ${ABS_API_DEF_PATH}/client-server)
set(ASAPI_DEF_DIR application-service/definitions)
set(ISAPI_DEF_DIR identity/definitions)
if (MATRIX_DOC_PATH AND GTAD_PATH)
    file(GLOB_RECURSE API_DEFS RELATIVE ${PROJECT_SOURCE_DIR}
        ${FULL_CSAPI_SRC_DIR}/*.yaml
        ${ABS_API_DEF_PATH}/${ASAPI_DEF_DIR}/*.yaml
        ${ABS_API_DEF_PATH}/${ISAPI_DEF_DIR}/*.yaml
    )
    add_custom_target(update-api
        ${ABS_GTAD_PATH} --config ${CSAPI_DIR}/gtad.yaml --out ${CSAPI_DIR}
            ${FULL_CSAPI_SRC_DIR}
            cas_login_redirect.yaml- cas_login_ticket.yaml-
            old_sync.yaml- room_initial_sync.yaml- # deprecated
            sync.yaml- # we have a better handcrafted implementation
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/lib
        SOURCES ${FULL_CSAPI_DIR}/gtad.yaml
                ${FULL_CSAPI_DIR}/{{base}}.h.mustache
                ${FULL_CSAPI_DIR}/{{base}}.cpp.mustache
                ${API_DEFS}
        VERBATIM
    )
endif()

aux_source_directory(${FULL_CSAPI_DIR} libqmatrixclient_job_SRCS)
aux_source_directory(${FULL_CSAPI_DIR}/definitions libqmatrixclient_csdef_SRCS)
aux_source_directory(${FULL_CSAPI_DIR}/definitions/wellknown libqmatrixclient_cswellknown_SRCS)
aux_source_directory(lib/${ASAPI_DEF_DIR} libqmatrixclient_asdef_SRCS)
aux_source_directory(lib/${ISAPI_DEF_DIR} libqmatrixclient_isdef_SRCS)

set(example_SRCS examples/qmc-example.cpp)

add_library(QMatrixClient ${libqmatrixclient_SRCS}
            ${libqmatrixclient_job_SRCS} ${libqmatrixclient_csdef_SRCS}
            ${libqmatrixclient_cswellknown_SRCS}
            ${libqmatrixclient_asdef_SRCS} ${libqmatrixclient_isdef_SRCS})
set(API_VERSION "0.4")
set_property(TARGET QMatrixClient PROPERTY VERSION "${API_VERSION}.0")
set_property(TARGET QMatrixClient PROPERTY SOVERSION ${API_VERSION} )
set_property(TARGET QMatrixClient PROPERTY
             INTERFACE_QMatrixClient_MAJOR_VERSION ${API_VERSION})
set_property(TARGET QMatrixClient APPEND PROPERTY
             COMPATIBLE_INTERFACE_STRING QMatrixClient_MAJOR_VERSION)

target_include_directories(QMatrixClient PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(QMatrixClient Qt5::Core Qt5::Network Qt5::Gui)

add_executable(qmc-example ${example_SRCS})
target_link_libraries(qmc-example Qt5::Core QMatrixClient)
configure_file(QMatrixClient.pc.in ${CMAKE_CURRENT_BINARY_DIR}/QMatrixClient.pc @ONLY NEWLINE_STYLE UNIX)

# Installation

install(TARGETS QMatrixClient EXPORT QMatrixClientTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY lib/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h")

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/QMatrixClient/QMatrixClientConfigVersion.cmake"
    VERSION ${API_VERSION}
    COMPATIBILITY AnyNewerVersion
)

export(PACKAGE QMatrixClient)
export(EXPORT QMatrixClientTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/QMatrixClient/QMatrixClientTargets.cmake")
configure_file(cmake/QMatrixClientConfig.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/QMatrixClient/QMatrixClientConfig.cmake"
    COPYONLY
)

set(ConfigFilesLocation "${CMAKE_INSTALL_LIBDIR}/cmake/QMatrixClient")
install(EXPORT QMatrixClientTargets
        FILE QMatrixClientTargets.cmake DESTINATION ${ConfigFilesLocation})

install(FILES cmake/QMatrixClientConfig.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/QMatrixClient/QMatrixClientConfigVersion.cmake"
    DESTINATION ${ConfigFilesLocation}
)
# Only available from CMake 3.7; reserved for future use
#install(EXPORT_ANDROID_MK QMatrixClientTargets DESTINATION share/ndk-modules)

if (WIN32)
    install(FILES mime/packages/freedesktop.org.xml DESTINATION mime/packages)
endif (WIN32)

install(TARGETS qmc-example RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if (UNIX AND NOT APPLE)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/QMatrixClient.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endif()
