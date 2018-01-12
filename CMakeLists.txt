cmake_minimum_required(VERSION 3.1)

project(qmatrixclient CXX)

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

set(CMAKE_CXX_STANDARD 14)

foreach (FLAG all "" pedantic extra error=return-type no-unused-parameter no-gnu-zero-variadic-macro-arguments)
    CHECK_CXX_COMPILER_FLAG("-W${FLAG}" WARN_${FLAG}_SUPPORTED)
    if ( WARN_${FLAG}_SUPPORTED AND NOT CMAKE_CXX_FLAGS MATCHES "(^| )-W?${FLAG}($| )")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W${FLAG}")
    endif ()
endforeach ()

find_package(Qt5 5.6 REQUIRED Network Gui)
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
if (MATRIX_DOC_PATH AND GTAD_PATH)
    message( STATUS "Generating API stubs enabled" )
    message( STATUS "  Using GTAD at ${GTAD_PATH}" )
    message( STATUS "  Using CS API files at ${MATRIX_DOC_PATH}/api/client-server" )
endif ()
message( STATUS "=============================================================================" )
message( STATUS )

# Set up source files
set(libqmatrixclient_SRCS
   networkaccessmanager.cpp
   connectiondata.cpp
   connection.cpp
   logging.cpp
   room.cpp
   user.cpp
   avatar.cpp
   settings.cpp
   networksettings.cpp
   events/event.cpp
   events/eventcontent.cpp
   events/roommessageevent.cpp
   events/roommemberevent.cpp
   events/roomavatarevent.cpp
   events/typingevent.cpp
   events/receiptevent.cpp
   jobs/requestdata.cpp
   jobs/basejob.cpp
   jobs/checkauthmethods.cpp
   jobs/sendeventjob.cpp
   jobs/setroomstatejob.cpp
   jobs/joinroomjob.cpp
   jobs/roommessagesjob.cpp
   jobs/syncjob.cpp
   jobs/mediathumbnailjob.cpp
)

set(API_DEF_PATH ${MATRIX_DOC_PATH}/api/client-server/)
file(GLOB_RECURSE API_DEFS RELATIVE ${PROJECT_SOURCE_DIR}
    ${API_DEF_PATH}/*.yaml
    ${API_DEF_PATH}/definitions/*.yaml
    ${MATRIX_DOC_PATH}/event-schemas/schema/*
)
if (MATRIX_DOC_PATH AND GTAD_PATH)
    add_custom_target(update-api
        ${GTAD_PATH} --config jobs/gtad.yaml --out jobs/generated
            ${MATRIX_DOC_PATH}/api/client-server
            cas_login_redirect.yaml- cas_login_ticket.yaml-
            old_sync.yaml- room_initial_sync.yaml-
            sync.yaml- room_state.yaml-
            event_context.yaml- joining.yaml-
            notifications.yaml- peeking_events.yaml-
            pushrules.yaml- rooms.yaml- search.yaml-
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        SOURCES jobs/gtad.yaml
                jobs/{{base}}.h.mustache jobs/{{base}}.cpp.mustache
                ${API_DEFS}
        VERBATIM
    )
endif()

aux_source_directory(jobs/generated libqmatrixclient_job_SRCS)

set(example_SRCS examples/qmc-example.cpp)

add_library(qmatrixclient ${libqmatrixclient_SRCS} ${libqmatrixclient_job_SRCS})
set_property(TARGET qmatrixclient PROPERTY VERSION "0.2.0")
set_property(TARGET qmatrixclient PROPERTY SOVERSION 0 )

target_link_libraries(qmatrixclient Qt5::Core Qt5::Network Qt5::Gui)

add_executable(qmc-example ${example_SRCS})
target_link_libraries(qmc-example Qt5::Core qmatrixclient)

if (WIN32)
    install (FILES mime/packages/freedesktop.org.xml
             DESTINATION mime/packages)
endif (WIN32)
