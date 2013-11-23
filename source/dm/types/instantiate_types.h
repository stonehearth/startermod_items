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

#include "dm/boxed.h"
#include "dm/set.h"
#include "dm/map.h"
#include "dm/array.h"
#include "dm/dm_save_impl.h"
#include "protocols/store.pb.h"

#include "basic_types.h"
#include "dm_types.h"
#include "om_types.h"
#include "csg_types.h"
#include "json_types.h"
#include "lua_types.h"
