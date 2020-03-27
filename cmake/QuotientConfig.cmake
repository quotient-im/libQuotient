include(CMakeFindDependencyMacro)

if (Quotient_ENABLE_E2EE)
    find_dependency(QtOlm)
endif()
include("${CMAKE_CURRENT_LIST_DIR}/QuotientTargets.cmake")
