#ifndef _RADIANT_LIB_TYPEINFO_TYPE_H
#define _RADIANT_LIB_TYPEINFO_TYPE_H

#include "typeinfo.h"
#include "dispatch_table.h"

#define DECLARE_TYPEINFO(Cls, clsid)                                          \
   template typeinfo::Type<Cls>;                                              \
   const typeinfo::TypeId typeinfo::Type<Cls>::id = (typeinfo::TypeId)clsid;  \
   bool typeinfo::Type<Cls>::registered = false;                              \

BEGIN_RADIANT_TYPEINFO_NAMESPACE

template <typename T>
class Type
{
public:
   static TypeId GetTypeId(){
      ASSERT(id);
      return id;
   }

public:
   static const TypeId     id;
   static bool             registered;
};

END_RADIANT_TYPEINFO_NAMESPACE

#endif // _RADIANT_LIB_TYPEINFO_TYPE_H
