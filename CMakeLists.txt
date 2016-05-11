cmake_minimum_required(VERSION 2.8.11) # Maybe works with even older versions

project(qmatrixclient)
enable_language(CXX)

include(CheckCXXCompilerFlag)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# Whether to build with the bundled KCoreAddons or system KCoreAddons
set( BUNDLE_KCOREADDONS "AUTO" CACHE STRING "Build own KCoreAddons, one of ON, OFF and AUTO" )
set( KCOREADDONS_DIR "kcoreaddons" CACHE STRING "Local path to bundled KCoreAddons sources, if own KCoreAddons is built" )

find_package(Qt5Core 5.3.0) # For JSON (de)serialization
find_package(Qt5Network 5.3.0) # For networking
find_package(Qt5Gui 5.3.0) # For userpics

if ( (NOT BUNDLE_KCOREADDONS STREQUAL "ON")
     AND (NOT BUNDLE_KCOREADDONS STREQUAL "OFF")
     AND (NOT BUNDLE_KCOREADDONS STREQUAL "AUTO") )
       message( FATAL_ERROR "BUNDLE_KCOREADDONS must be one of ON, OFF or AUTO" )
endif ()

if ( BUNDLE_KCOREADDONS STREQUAL "AUTO" )
    find_package(KF5CoreAddons QUIET)
elseif ( BUNDLE_KCOREADDONS STREQUAL "OFF" )
    find_package(KF5CoreAddons REQUIRED)
endif ()

message( STATUS )
message( STATUS "================================================================================" )
message( STATUS "                          libqmatrixclient Build Information                          " )
message( STATUS "================================================================================" )
message( STATUS "Building with: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}" )
message( STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}" )
message( STATUS "Path to Qt Core: ${Qt5Core_DIR}" )
message( STATUS "Build own KCoreAddons (BUNDLE_KCOREADDONS): ${BUNDLE_KCOREADDONS}" )
if ( NOT BUNDLE_KCOREADDONS STREQUAL "ON" )
    if ( KF5CoreAddons_FOUND )
        message( STATUS "'- Path to system KCoreAddons: ${KF5CoreAddons_DIR}" )
    else ( KF5CoreAddons_FOUND )
        message( STATUS "'- System KCoreAddons not found, using the bundled version at ${PROJECT_SOURCE_DIR}/${KCOREADDONS_DIR}" )
    endif ( KF5CoreAddons_FOUND )
endif ( NOT BUNDLE_KCOREADDONS STREQUAL "ON" )
message( STATUS "================================================================================" )
message( STATUS )

# Set up source files
set(libqmatrixclient_SRCS
   connectiondata.cpp
   connection.cpp
   connectionprivate.cpp
   room.cpp
   user.cpp
   logmessage.cpp
   state.cpp
   events/event.cpp
   events/roommessageevent.cpp
   events/roomnameevent.cpp
   events/roomaliasesevent.cpp
   events/roomcanonicalaliasevent.cpp
   events/roommemberevent.cpp
   events/roomtopicevent.cpp
   events/typingevent.cpp
   events/receiptevent.cpp
   events/unknownevent.cpp
   jobs/basejob.cpp
   jobs/simplejob.cpp
   jobs/checkauthmethods.cpp
   jobs/passwordlogin.cpp
   jobs/postmessagejob.cpp
   jobs/postreceiptjob.cpp
   jobs/joinroomjob.cpp
   jobs/leaveroomjob.cpp
   jobs/roommembersjob.cpp
   jobs/roommessagesjob.cpp
   jobs/syncjob.cpp
   jobs/mediathumbnailjob.cpp
    )
# Add bundled KCoreAddons sources if we haven't found the system sources
# or if we ignore them
if ( NOT KF5CoreAddons_FOUND )
    set (libqmatrixclient_SRCS ${libqmatrixclient_SRCS}
        ${KCOREADDONS_DIR}/src/lib/jobs/kjob.cpp
        ${KCOREADDONS_DIR}/src/lib/jobs/kcompositejob.cpp
        ${KCOREADDONS_DIR}/src/lib/jobs/kjobtrackerinterface.cpp
        ${KCOREADDONS_DIR}/src/lib/jobs/kjobuidelegate.cpp
        )
endif ( NOT KF5CoreAddons_FOUND )

add_library(qmatrixclient ${libqmatrixclient_SRCS})

if ( CMAKE_VERSION VERSION_LESS "3.1" )
    CHECK_CXX_COMPILER_FLAG("-std=c++11" STD_FLAG_SUPPORTED)
    if ( STD_FLAG_SUPPORTED )
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    endif ( STD_FLAG_SUPPORTED )
else ( CMAKE_VERSION VERSION_LESS "3.1" )
    target_compile_features(qmatrixclient PRIVATE cxx_range_for)
    target_compile_features(qmatrixclient PRIVATE cxx_override)
    target_compile_features(qmatrixclient PRIVATE cxx_strong_enums)
    target_compile_features(qmatrixclient PRIVATE cxx_lambdas)
    target_compile_features(qmatrixclient PRIVATE cxx_auto_type)
    target_compile_features(qmatrixclient PRIVATE cxx_generalized_initializers)
    target_compile_features(qmatrixclient PRIVATE cxx_nullptr)
endif ( CMAKE_VERSION VERSION_LESS "3.1" )

target_link_libraries(qmatrixclient Qt5::Core Qt5::Network Qt5::Gui)
if ( KF5CoreAddons_FOUND )
    # The proper way of doing things would be to make a separate config.h.in
    # file and use configure_file() command here to generate config.h with
    # needed C++ preprocessor macros. If we have more than one or two
    # dependencies like that, we should turn to that more scalable way.
    # As for now, passing a macro through -D is easier to observe and maintain.
    target_compile_definitions ( qmatrixclient PRIVATE USING_SYSTEM_KCOREADDONS )
    target_link_libraries(qmatrixclient KF5::CoreAddons)
else ( KF5CoreAddons_FOUND )
    include_directories( ${KCOREADDONS_DIR}/src/lib/jobs )
endif ( KF5CoreAddons_FOUND )
