cmake_minimum_required(VERSION 2.8.11) # Maybe works with even older versions

project(qmatrixclient)
enable_language(CXX)

include(CheckCXXCompilerFlag)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
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

if ( CMAKE_VERSION VERSION_LESS "3.1" )
    CHECK_CXX_COMPILER_FLAG("-std=c++11" STD_FLAG_SUPPORTED)
    if ( STD_FLAG_SUPPORTED )
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    endif ( STD_FLAG_SUPPORTED )
else ( CMAKE_VERSION VERSION_LESS "3.1" )
    set(CMAKE_CXX_STANDARD 11)
endif ( CMAKE_VERSION VERSION_LESS "3.1" )

find_package(Qt5 5.2.1 REQUIRED Network Gui)
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

aux_source_directory(serverapi/generated libqmatrixclient_SRCS)
aux_source_directory(serverapi/generated/definitions libqmatrixclient_SRCS)

# Set up source files
list(APPEND libqmatrixclient_SRCS
   connectiondata.cpp
   connection.cpp
   room.cpp
   user.cpp
   state.cpp
   settings.cpp
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
   serverapi/servercallsetup.cpp
   serverapi/servercall.cpp
   serverapi/passwordlogin.cpp
   serverapi/logout.cpp
   serverapi/getmediathumbnail.cpp
   serverapi/joinroom.cpp
   jobs/basejob.cpp
   jobs/checkauthmethods.cpp
   jobs/postmessagejob.cpp
   jobs/postreceiptjob.cpp
   jobs/leaveroomjob.cpp
   jobs/roommessagesjob.cpp
   jobs/syncjob.cpp
)
set(example_SRCS examples/qmc-example.cpp)

add_library(qmatrixclient ${libqmatrixclient_SRCS})
set_property(TARGET qmatrixclient PROPERTY VERSION "0.0.0")
set_property(TARGET qmatrixclient PROPERTY SOVERSION 0 )

target_link_libraries(qmatrixclient Qt5::Core Qt5::Network Qt5::Gui)

add_executable(qmc-example ${example_SRCS})
target_link_libraries(qmc-example Qt5::Core qmatrixclient)
