#ifndef _RADIANT_H
#define _RADIANT_H

#if defined(WIN32)
#  include "radiant_win32.h"
#  define WIN32_ONLY(x)    x
#else
#  error "Unsupported platform in radiant.h"
#endif

#include "radiant_exceptions.h"
#include "radiant_assert.h"
#include "radiant_types.h"
#include "radiant_macros.h"
#include "radiant_logger.h"

#endif // _RADIANT_H


