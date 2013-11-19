// boilerplate intentionally omitted...
#include "om/reflection/clear.h"

#define OM_BEGIN_CLASS(Cls, cls, Base)   DEFINE_OM_OBJECT_TYPE(Cls, cls);
#define OM_END_CLASS(Cls, cls, Base)

#define OM_PROPERTY_SET(Cls, lname, Uname, Type)    Cls& Set ## Uname(Type const& value) { lname ## _ = value; return *this; }
#define OM_PROPERTY_GET(Cls, lname, Uname, Type)    Type const& Get ## Uname() const { return *(lname ## _); }

#define OM_PROPERTY(Cls, lname, Uname, Type)    \
   OM_PROPERTY_SET(Cls, lname, Uname, Type)     \
   OM_PROPERTY_GET(Cls, lname, Uname, Type)

