# - Try to find LibOlm

# Uses the following variables to help find libolm:
#  Olm_INCLUDE_DIR - include files
#  Olm_LIBRARY_DIR - libraries
# Once done this will define
#  Olm_FOUND - System has olm
#  Olm_INCLUDE_DIRS - The olm include directories
#  Olm_LIBRARIES - The libraries needed to use olm

find_path(Olm_INCLUDE_DIRS NAMES
    olm/olm.h
    olm/inbound_group_session.h
    olm/outbound_group_session.h
    PATHS "${Olm_INCLUDE_DIR}"
    DOC "Path to a directory with libolm header files"
)

find_library(Olm_LIBRARIES NAMES olm
    PATHS "${Olm_LIBRARY_DIR}"
    DOC "Path to a directory with libolm libraries"
)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set OLM_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(olm DEFAULT_MSG
                                  Olm_LIBRARIES Olm_INCLUDE_DIRS)

mark_as_advanced(Olm_INCLUDE_DIRS Olm_LIBRARIES)
