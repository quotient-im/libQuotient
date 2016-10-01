# - Try to find LibOlm
# Once done this will define
#  OLM_FOUND - System has olm
#  OLM_INCLUDE_DIRS - The olm include directories
#  OLM_LIBRARIES - The libraries needed to use olm

#include(LibFindMacros)

find_path(OLM_INCLUDE_DIR 
          NAMES olm/olm.h olm/inbound_group_session.h olm/outbound_group_session.h
         )

find_library(OLM_LIBRARY NAMES olm
            )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBXML2_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(olm  DEFAULT_MSG
                                  OLM_LIBRARY OLM_INCLUDE_DIR)

mark_as_advanced(OLM_INCLUDE_DIR OLM_LIBRARY )

set(OLM_LIBRARIES ${OLM_LIBRARY} )
set(OLM_INCLUDE_DIRS ${OLM_INCLUDE_DIR} )
