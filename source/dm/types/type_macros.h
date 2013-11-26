#ifndef _DM_TYPES_TYPE_MACROS_H
#define _DM_TYPES_TYPE_MACROS_H

#include "dm/dm.h"

#ifndef CREATE_BOXED
#  define CREATE_BOXED(B)
#endif 

#ifndef CREATE_MAP
#  define CREATE_MAP(M)
#endif

#ifndef CREATE_SET
#  define CREATE_SET(S)
#endif

#define UNWRAP2(x, y)      x, y
#define UNWRAP3(x, y, z)   x, y, z

#define SET(v)          CREATE_SET(dm::Set<v>)
#define BOXED(v)        CREATE_BOXED(dm::Boxed<v>)
#define MAP(k, v)       CREATE_MAP(dm::Map<UNWRAP2(k, v)>)
#define MAP3(k, v, h)   CREATE_MAP(dm::Map<UNWRAP3(k, v, h)>)

#endif // _DM_TYPES_TYPE_MACROS_H
