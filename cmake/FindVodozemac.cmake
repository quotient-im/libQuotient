# SPDX-FileCopyrightText: 2024 Tobias Fella <tobias.fella@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

find_library(VODOZEMAC_LIB REQUIRED NAMES vodozemac)
find_path(VODOZEMAC_INCLUDE_DIR REQUIRED NAMES vodozemac/vodozemac.h)

add_library(Vodozemac UNKNOWN IMPORTED)
set_target_properties(Vodozemac PROPERTIES IMPORTED_LOCATION ${VODOZEMAC_LIB})
set_target_properties(Vodozemac PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${VODOZEMAC_INCLUDE_DIR})
set(Vodozemac_FOUND True)
