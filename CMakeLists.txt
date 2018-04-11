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

find_package(Qt5 5.5.1 REQUIRED Network Gui)
get_filename_component(Qt5_Prefix "${Qt5_DIR}/../../../.." ABSOLUTE)

message( STATUS )
message( STATUS "=============================================================================" )
message( STATUS "                       libqmatrixclient Build Information" )
message( STATUS "=============================================================================" )
if (CMAKE_BUILD_TYPE)
    message( STATUS "Build type: ${CMAKE_BUILD_TYPE}")
endif(CMAKE_BUILD_TYPE)
message( STATUS "Using compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}" )
message( STATUS "Using Qt ${Qt5_VERSION} at ${Qt5_Prefix}" )
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
    lib/events/event.cpp
    lib/events/eventcontent.cpp
    lib/events/roommessageevent.cpp
    lib/events/roommemberevent.cpp
    lib/events/roomavatarevent.cpp
    lib/events/typingevent.cpp
    lib/events/receiptevent.cpp
    lib/events/directchatevent.cpp
    lib/jobs/requestdata.cpp
    lib/jobs/basejob.cpp
    lib/jobs/checkauthmethods.cpp
    lib/jobs/sendeventjob.cpp
    lib/jobs/setroomstatejob.cpp
    lib/jobs/joinroomjob.cpp
    lib/jobs/roommessagesjob.cpp
    lib/jobs/syncjob.cpp
    lib/jobs/mediathumbnailjob.cpp
    lib/jobs/downloadfilejob.cpp
)

aux_source_directory(lib/jobs/generated libqmatrixclient_job_SRCS)

set(example_SRCS examples/qmc-example.cpp)

add_library(QMatrixClient ${libqmatrixclient_SRCS} ${libqmatrixclient_job_SRCS})
set(API_VERSION "0.2")
set_property(TARGET QMatrixClient PROPERTY VERSION "${API_VERSION}.0")
set_property(TARGET QMatrixClient PROPERTY SOVERSION 0 )
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
