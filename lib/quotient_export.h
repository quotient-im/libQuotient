// SPDX-FileCopyrightText: 2021 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QtCore/qglobal.h>

#ifdef QUOTIENT_STATIC
#  define QUOTIENT_API
#  define QUOTIENT_HIDDEN
#else
#  ifndef QUOTIENT_API
#    ifdef BUILDING_SHARED_QUOTIENT
        /* We are building this library */
#      define QUOTIENT_API Q_DECL_EXPORT
#    else
        /* We are using this library */
#      define QUOTIENT_API Q_DECL_IMPORT
#    endif
#  endif

#  ifndef QUOTIENT_HIDDEN
#    define QUOTIENT_HIDDEN Q_DECL_HIDDEN
#  endif
#endif
