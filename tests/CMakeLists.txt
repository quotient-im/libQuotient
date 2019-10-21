cmake_minimum_required(VERSION 3.1)

# This CMakeLists file assumes that the library is installed to CMAKE_INSTALL_PREFIX
# and ignores the in-tree library code. You can use this to start work on your own client.

project(quotest CXX)

include(CheckCXXCompilerFlag)
if (NOT WIN32)
    include(GNUInstallDirs)
endif(NOT WIN32)

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

find_package(Qt5 5.9 REQUIRED Network Gui Multimedia Test)
get_filename_component(Qt5_Prefix "${Qt5_DIR}/../../../.." ABSOLUTE)

find_package(Quotient REQUIRED)
get_filename_component(Quotient_Prefix "${Quotient_DIR}/../.." ABSOLUTE)

message( STATUS "${PROJECT_NAME} configuration:" )
if (CMAKE_BUILD_TYPE)
    message( STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
endif(CMAKE_BUILD_TYPE)
message( STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}" )
message( STATUS "  Qt: ${Qt5_VERSION} at ${Qt5_Prefix}" )
message( STATUS "  Quotient: ${Quotient_VERSION} at ${Quotient_Prefix}" )

set(example_SRCS quotest.cpp)

add_executable(${PROJECT_NAME} ${example_SRCS})
target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Test Quotient)

# Installation

install (TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
