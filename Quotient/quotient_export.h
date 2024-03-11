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
#      ifdef Q_OS_WIN
#        define QUOTIENT_API Q_DECL_EXPORT
#      else
         // On non-Windows, Q_DECL_EXPORT can apply protected visibility and the current code for
         // event types is incompatible with it (see #692).
#        define QUOTIENT_API __attribute__((visibility("default")))
#      endif
#    else
        /* We are using this library */
#      define QUOTIENT_API Q_DECL_IMPORT
#    endif
#  endif

#  ifndef QUOTIENT_HIDDEN
#    define QUOTIENT_HIDDEN Q_DECL_HIDDEN
#  endif
#endif
