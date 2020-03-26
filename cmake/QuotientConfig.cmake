include(CMakeFindDependencyMacro)

if (Quotient_E2EE_ENABLED)
    find_dependency(QtOlm)
endif()
include("${CMAKE_CURRENT_LIST_DIR}/QuotientTargets.cmake")
