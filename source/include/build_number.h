#ifndef _BUILD_NUMBER_H
#define _BUILD_NUMBER_H

#include "build_defaults.h"

#if defined(USE_BUILD_OVERRIDES)
#   include "build_overrides.h"
#endif

#define PRODUCT_VERSION_STR         #PRODUCT_MAJOR_VERSION "." #PRODUCT_MINOR_VERSION "." #PRODUCT_PATCH_VERSION
#define PRODUCT_FILE_VERSION_STR    #PRODUCT_VERSION_STR "." #PRODUCT_BUILD_NUMBER

#endif // _BUILD_NUMBER_H
