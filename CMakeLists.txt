cmake_minimum_required(VERSION 3.26)
if (POLICY CMP0092)
cmake_policy(SET CMP0092 NEW)
endif()

set(API_VERSION "0.9")
project(Quotient VERSION "${API_VERSION}.0" LANGUAGES CXX)
set(PRE_STAGE "alpha")
set(FULL_VERSION ${PROJECT_VERSION}~${PRE_STAGE})

message(STATUS)
message(STATUS "Configuring ${PROJECT_NAME} ${FULL_VERSION} ==>")

include(FeatureSummary)
include(CTest)

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Debug' as none was specified")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build" FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()
include(CheckCXXCompilerFlag)
if (MSVC)
    add_compile_options(/EHsc /W4
        /wd4100 /wd4127 /wd4242 /wd4244 /wd4245 /wd4267 /wd4365 /wd4456 /wd4459
        /wd4464 /wd4505 /wd4514 /wd4571 /wd4619 /wd4623 /wd4625 /wd4626 /wd4706
        /wd4710 /wd4774 /wd4820 /wd4946 /wd5026 /wd5027 /wd4702)
else()
    foreach (FLAG all pedantic extra error=return-type) # Switch these on
        CHECK_CXX_COMPILER_FLAG("-W${FLAG}" W${FLAG}_SUPPORTED)
        if (W${FLAG}_SUPPORTED AND
                NOT CMAKE_CXX_FLAGS MATCHES "W(no-)?${FLAG}($| )")
            add_compile_options(-W${FLAG})
        endif ()
    endforeach ()
    foreach (FLAG unused-parameter gnu-zero-variadic-macro-arguments
                  subobject-linkage) # Switch these off
        CHECK_CXX_COMPILER_FLAG("-Wno-${FLAG}" Wno-${FLAG}_SUPPORTED)
        if (Wno-${FLAG}_SUPPORTED AND
                NOT CMAKE_CXX_FLAGS MATCHES "W(no-)?${FLAG}($| )")
            add_compile_options(-Wno-${FLAG})
        endif()
    endforeach ()
    # disable exceptions by default, not used in most parts here and saves about 10% binary size
    add_compile_options(-fno-exceptions)
endif()

add_compile_definitions(
    QT_NO_JAVA_STYLE_ITERATORS QT_NO_URL_CAST_FROM_STRING QT_NO_CAST_TO_ASCII
    QT_NO_CAST_FROM_ASCII QT_STRICT_ITERATORS QT_NO_CAST_FROM_BYTEARRAY
    QT_NO_NARROWING_CONVERSIONS_IN_CONNECT QT_NO_FOREACH)

set_directory_properties(PROPERTIES
    CXX_STANDARD 23
    CXX_EXTENSIONS OFF
    VISIBILITY_INLINES_HIDDEN ON
    CXX_VISIBILITY_PRESET hidden
)

set(QtMinVersion "6.4")
string(REGEX REPLACE "^(.).*" "Qt\\1" Qt ${QtMinVersion}) # makes "Qt<MajorVersionDigit>"
set(QUOTIENT_LIB_NAME ${PROJECT_NAME}${Qt})

if (WIN32)
    if (NOT CMAKE_INSTALL_LIBDIR)
        set(CMAKE_INSTALL_LIBDIR ".")
        set(CMakeFilesLocation "cmake")
    else()
        set(CMakeFilesLocation "${CMAKE_INSTALL_LIBDIR}/cmake/${QUOTIENT_LIB_NAME}")
    endif()

    if (NOT CMAKE_INSTALL_BINDIR)
        set(CMAKE_INSTALL_BINDIR ".")
    endif()

    if (NOT CMAKE_INSTALL_INCLUDEDIR)
        set(CMAKE_INSTALL_INCLUDEDIR "include")
    endif()
else()
    include(GNUInstallDirs)
    set(CMakeFilesLocation "${CMAKE_INSTALL_LIBDIR}/cmake/${QUOTIENT_LIB_NAME}")
endif(WIN32)
set(${PROJECT_NAME}_INSTALL_INCLUDEDIR
        "${CMAKE_INSTALL_INCLUDEDIR}/" CACHE PATH
        "directory to install ${PROJECT_NAME} include files to")

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(${Qt} ${QtMinVersion} REQUIRED Core Network Gui Test Sql)
get_filename_component(Qt_Prefix "${${Qt}_DIR}/../../../.." ABSOLUTE)

find_package(${Qt}Keychain REQUIRED)

find_package(Olm 3.2.5 REQUIRED)
set_package_properties(Olm PROPERTIES
    DESCRIPTION "Implementation of the Olm and Megolm cryptographic ratchets"
    URL "https://gitlab.matrix.org/matrix-org/olm"
    TYPE REQUIRED
)

find_package(OpenSSL 1.1.0 REQUIRED)
set_package_properties(OpenSSL PROPERTIES
    DESCRIPTION "Open source SSL and TLS implementation and cryptographic library"
    URL "https://www.openssl.org/"
    TYPE REQUIRED
)

add_library(${QUOTIENT_LIB_NAME})

target_sources(${QUOTIENT_LIB_NAME} PUBLIC FILE_SET HEADERS BASE_DIRS .
    FILES
        Quotient/quotient_common.h
        Quotient/quotient_export.h
        Quotient/function_traits.h
        Quotient/omittable.h
        Quotient/expected.h
        Quotient/networkaccessmanager.h
        Quotient/connectiondata.h
        Quotient/connection.h
        Quotient/connection_p.h
        Quotient/ssosession.h
        Quotient/logging_categories_p.h
        Quotient/room.h
        Quotient/roomstateview.h
        Quotient/user.h
        Quotient/roommember.h
        Quotient/avatar.h
        Quotient/uri.h
        Quotient/uriresolver.h
        Quotient/eventstats.h
        Quotient/syncdata.h
        Quotient/settings.h
        Quotient/networksettings.h
        Quotient/converters.h
        Quotient/util.h
        Quotient/eventitem.h
        Quotient/accountregistry.h
        Quotient/mxcreply.h
        Quotient/events/event.h
        Quotient/events/roomevent.h
        Quotient/events/stateevent.h
        Quotient/events/single_key_value.h
        Quotient/events/simplestateevents.h
        Quotient/events/eventcontent.h
        Quotient/events/eventrelation.h
        Quotient/events/roomcreateevent.h
        Quotient/events/roomtombstoneevent.h
        Quotient/events/roommessageevent.h
        Quotient/events/roommemberevent.h
        Quotient/events/roomcanonicalaliasevent.h
        Quotient/events/roomavatarevent.h
        Quotient/events/roompowerlevelsevent.h
        Quotient/events/typingevent.h
        Quotient/events/accountdataevents.h
        Quotient/events/receiptevent.h
        Quotient/events/reactionevent.h
        Quotient/events/callevents.h
        Quotient/events/directchatevent.h
        Quotient/events/encryptionevent.h
        Quotient/events/encryptedevent.h
        Quotient/events/roomkeyevent.h
        Quotient/events/stickerevent.h
        Quotient/events/filesourceinfo.h
        Quotient/jobs/requestdata.h
        Quotient/jobs/basejob.h
        Quotient/jobs/jobhandle.h
        Quotient/jobs/syncjob.h
        Quotient/jobs/mediathumbnailjob.h
        Quotient/jobs/downloadfilejob.h
        Quotient/database.h
        Quotient/connectionencryptiondata_p.h
        Quotient/keyverificationsession.h
        Quotient/e2ee/e2ee_common.h
        Quotient/e2ee/qolmaccount.h
        Quotient/e2ee/qolmsession.h
        Quotient/e2ee/qolminboundsession.h
        Quotient/e2ee/qolmoutboundsession.h
        Quotient/e2ee/qolmutility.h
        Quotient/e2ee/qolmsession.h
        Quotient/e2ee/qolmmessage.h
        Quotient/e2ee/cryptoutils.h
        Quotient/e2ee/sssshandler.h
        Quotient/events/keyverificationevent.h
    PRIVATE
        Quotient/function_traits.cpp
        Quotient/networkaccessmanager.cpp
        Quotient/connectiondata.cpp
        Quotient/connection.cpp
        Quotient/ssosession.cpp
        Quotient/room.cpp
        Quotient/roomstateview.cpp
        Quotient/user.cpp
        Quotient/roommember.cpp
        Quotient/avatar.cpp
        Quotient/uri.cpp
        Quotient/uriresolver.cpp
        Quotient/eventstats.cpp
        Quotient/syncdata.cpp
        Quotient/settings.cpp
        Quotient/networksettings.cpp
        Quotient/converters.cpp
        Quotient/util.cpp
        Quotient/eventitem.cpp
        Quotient/accountregistry.cpp
        Quotient/mxcreply.cpp
        Quotient/events/event.cpp
        Quotient/events/roomevent.cpp
        Quotient/events/stateevent.cpp
        Quotient/events/eventcontent.cpp
        Quotient/events/eventrelation.cpp
        Quotient/events/roomcreateevent.cpp
        Quotient/events/roomtombstoneevent.cpp
        Quotient/events/roommessageevent.cpp
        Quotient/events/roommemberevent.cpp
        Quotient/events/roompowerlevelsevent.cpp
        Quotient/events/receiptevent.cpp
        Quotient/events/callevents.cpp
        Quotient/events/directchatevent.cpp
        Quotient/events/encryptionevent.cpp
        Quotient/events/encryptedevent.cpp
        Quotient/events/filesourceinfo.cpp
        Quotient/jobs/requestdata.cpp
        Quotient/jobs/basejob.cpp
        Quotient/jobs/syncjob.cpp
        Quotient/jobs/mediathumbnailjob.cpp
        Quotient/jobs/downloadfilejob.cpp
        Quotient/database.cpp
        Quotient/connectionencryptiondata_p.cpp
        Quotient/keyverificationsession.cpp
        Quotient/e2ee/e2ee_common.cpp
        Quotient/e2ee/qolmaccount.cpp
        Quotient/e2ee/qolmsession.cpp
        Quotient/e2ee/qolminboundsession.cpp
        Quotient/e2ee/qolmoutboundsession.cpp
        Quotient/e2ee/qolmutility.cpp
        Quotient/e2ee/qolmsession.cpp
        Quotient/e2ee/qolmmessage.cpp
        Quotient/e2ee/cryptoutils.cpp
        Quotient/e2ee/sssshandler.cpp
        libquotientemojis.qrc
)

# Configure API files generation

set(CSAPI_DIR csapi)
set(FULL_CSAPI_DIR Quotient/${CSAPI_DIR})
set(ASAPI_DEF_DIR application-service/definitions)

set(API_GENERATION_ENABLED 0)
if (NOT MATRIX_SPEC_PATH AND MATRIX_DOC_PATH)
    set(MATRIX_SPEC_PATH ${MATRIX_DOC_PATH})
endif()
if (GTAD_PATH AND MATRIX_SPEC_PATH)
    # REALPATH resolves ~ (home directory) while PROGRAM doesn't
    get_filename_component(ABS_GTAD_PATH "${GTAD_PATH}" REALPATH)
    get_filename_component(ABS_GTAD_PATH "${ABS_GTAD_PATH}" PROGRAM PROGRAM_ARGS GTAD_ARGS)
    if (EXISTS ${ABS_GTAD_PATH})
        get_filename_component(ABS_API_DEF_PATH "${MATRIX_SPEC_PATH}/data/api" REALPATH)
        if (NOT IS_DIRECTORY ${ABS_API_DEF_PATH})
            # Check the old place of API files
            get_filename_component(ABS_API_DEF_PATH "${MATRIX_SPEC_PATH}/api" REALPATH)
        endif ()
        if (IS_DIRECTORY ${ABS_API_DEF_PATH})
            set(API_GENERATION_ENABLED 1)
        else ()
            message( WARNING "${MATRIX_SPEC_PATH} doesn't seem to point to a valid matrix-doc repo; disabling API stubs generation")
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
    )
    add_custom_target(update-api
        ${ABS_GTAD_PATH} --config ../gtad/gtad.yaml --out ${CSAPI_DIR}
            ${FULL_CSAPI_SRC_DIR}
            old_sync.yaml- room_initial_sync.yaml- # deprecated
            sync.yaml- # we have a better handcrafted implementation
            ${GTAD_ARGS}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/Quotient
        SOURCES gtad/gtad.yaml
                gtad/data.h.mustache
                gtad/operation.h.mustache
                gtad/operation.cpp.mustache
                gtad/preamble.mustache
                gtad/define_models.mustache
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

file(GLOB_RECURSE api_ALL_HEADERS CONFIGURE_DEPENDS ${FULL_CSAPI_DIR}/*.h Quotient/${ASAPI_DEF_DIR}/*.h)
target_sources(${QUOTIENT_LIB_NAME} PUBLIC FILE_SET HEADERS FILES ${api_ALL_HEADERS})

file(GLOB_RECURSE api_ALL_IMPLS CONFIGURE_DEPENDS ${FULL_CSAPI_DIR}/*.cpp)
target_sources(${QUOTIENT_LIB_NAME} PRIVATE ${api_ALL_IMPLS})

# Set BUILDING_SHARED_QUOTIENT if building as a shared library
target_compile_definitions(${QUOTIENT_LIB_NAME} PRIVATE
    $<$<STREQUAL:$<TARGET_PROPERTY:${QUOTIENT_LIB_NAME},TYPE>,SHARED_LIBRARY>:BUILDING_SHARED_QUOTIENT>)
# Set QUOTIENT_STATIC in a static library setting
target_compile_definitions(${QUOTIENT_LIB_NAME} PUBLIC
    $<$<STREQUAL:$<TARGET_PROPERTY:${QUOTIENT_LIB_NAME},TYPE>,STATIC_LIBRARY>:QUOTIENT_STATIC>)
target_compile_definitions(${QUOTIENT_LIB_NAME} PUBLIC ${PROJECT_NAME}_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    ${PROJECT_NAME}_VERSION_MINOR=${PROJECT_VERSION_MINOR} ${PROJECT_NAME}_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    ${PROJECT_NAME}_VERSION_STRING=\"${PROJECT_VERSION}\" ${PROJECT_NAME}_PRE_STAGE=\"${PRE_STAGE}\"
    ${PROJECT_NAME}_E2EE_ENABLED)
set_target_properties(${QUOTIENT_LIB_NAME} PROPERTIES
    VERSION "${PROJECT_VERSION}"
    SOVERSION ${API_VERSION}
    INTERFACE_${PROJECT_NAME}_MAJOR_VERSION ${API_VERSION}
)
set_property(TARGET ${QUOTIENT_LIB_NAME} APPEND PROPERTY
             COMPATIBLE_INTERFACE_STRING ${PROJECT_NAME}_MAJOR_VERSION)

target_compile_features(${QUOTIENT_LIB_NAME} PUBLIC cxx_std_23)
if (MSVC)
    target_compile_options(${QUOTIENT_LIB_NAME} PUBLIC /Zc:preprocessor)
endif()

# Don't use PCH w/GCC (https://bugzilla.redhat.com/show_bug.cgi?id=1721553#c34)
if (NOT CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    target_precompile_headers(${QUOTIENT_LIB_NAME} PRIVATE Quotient/converters.h)
endif ()

target_include_directories(${QUOTIENT_LIB_NAME} PUBLIC
    $<BUILD_INTERFACE:$<$<NOT:$<BOOL:${QUOTIENT_FORCE_NAMESPACED_INCLUDES}>>:${CMAKE_CURRENT_SOURCE_DIR}/Quotient>>
)

target_link_libraries(${QUOTIENT_LIB_NAME}
    PUBLIC ${Qt}::Core ${Qt}::Network ${Qt}::Gui qt${${Qt}Core_VERSION_MAJOR}keychain Olm::Olm ${Qt}::Sql
    PRIVATE OpenSSL::Crypto)

configure_file(${PROJECT_NAME}.pc.in ${CMAKE_CURRENT_BINARY_DIR}/${QUOTIENT_LIB_NAME}.pc @ONLY NEWLINE_STYLE UNIX)

# Configure testing

if (BUILD_TESTING)
    enable_testing()
    add_subdirectory(quotest)
    add_subdirectory(autotests)
endif()

# Configure installation

install(TARGETS ${QUOTIENT_LIB_NAME} EXPORT ${QUOTIENT_LIB_NAME}Targets
    RUNTIME LIBRARY ARCHIVE FILE_SET HEADERS)

include(CMakePackageConfigHelpers)
# NB: SameMajorVersion doesn't really work yet, as we're within 0.x trail.
# Maybe consider jumping the gun and releasing 1.0, as semver advises?
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${QUOTIENT_LIB_NAME}/${QUOTIENT_LIB_NAME}ConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
)

export(PACKAGE ${QUOTIENT_LIB_NAME})
export(EXPORT ${QUOTIENT_LIB_NAME}Targets
       FILE "${CMAKE_CURRENT_BINARY_DIR}/${QUOTIENT_LIB_NAME}/${QUOTIENT_LIB_NAME}Targets.cmake")

# make sure BUILD_SHARED_LIBS is always defined for generating the QuotientConfig.cmake file
if (NOT DEFINED BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS OFF)
endif()
configure_file(cmake/${PROJECT_NAME}Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/${QUOTIENT_LIB_NAME}/${QUOTIENT_LIB_NAME}Config.cmake"
    @ONLY
)

install(EXPORT ${QUOTIENT_LIB_NAME}Targets
        FILE ${QUOTIENT_LIB_NAME}Targets.cmake DESTINATION ${CMakeFilesLocation})

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${QUOTIENT_LIB_NAME}/${QUOTIENT_LIB_NAME}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${QUOTIENT_LIB_NAME}/${QUOTIENT_LIB_NAME}ConfigVersion.cmake"
    DESTINATION ${CMakeFilesLocation}
)
install(EXPORT_ANDROID_MK ${QUOTIENT_LIB_NAME}Targets DESTINATION ${CMAKE_INSTALL_DATADIR}/ndk-modules)

if (WIN32)
    install(FILES mime/packages/freedesktop.org.xml DESTINATION mime/packages)
endif (WIN32)

if (UNIX AND NOT APPLE)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${QUOTIENT_LIB_NAME}.pc
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endif()

message(STATUS)
message(STATUS "== libQuotient ${FULL_VERSION} configuration summary ==")
message(STATUS)
if (CMAKE_BUILD_TYPE)
    message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
endif(CMAKE_BUILD_TYPE)
message(STATUS "Using compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}" )
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Header files install prefix: ${CMAKE_INSTALL_PREFIX}/${${PROJECT_NAME}_INSTALL_INCLUDEDIR}")
message(STATUS "Using Qt ${${Qt}_VERSION} at ${Qt_Prefix}")
message(STATUS "Using QtKeychain ${${Qt}Keychain_VERSION} at ${${Qt}Keychain_DIR}")
message(STATUS "Using libOlm ${Olm_VERSION} at ${Olm_DIR}")
message(STATUS "Using OpenSSL libcrypto ${OPENSSL_VERSION} at ${OPENSSL_CRYPTO_LIBRARY}")
message(STATUS)
feature_summary(WHAT ENABLED_FEATURES DISABLED_FEATURES
                FATAL_ON_MISSING_REQUIRED_PACKAGES)

message(STATUS "<== End of libQuotient configuration")
