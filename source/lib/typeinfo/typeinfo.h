#ifndef _RADIANT_LIB_TYPEINFO_H
#define _RADIANT_LIB_TYPEINFO_H

#include "protocols/forward_defines.h"
#include "lib/lua/bind_forward_declarations.h"
#include "dm/dm.h"

#define BEGIN_RADIANT_TYPEINFO_NAMESPACE  namespace radiant { namespace typeinfo {
#define END_RADIANT_TYPEINFO_NAMESPACE    } }

BEGIN_RADIANT_TYPEINFO_NAMESPACE

typedef int TypeId;
class Dispatcher;
class DispatchTable;
template <class T> class Type;

END_RADIANT_TYPEINFO_NAMESPACE

#endif // _RADIANT_LIB_TYPEINFO_H
