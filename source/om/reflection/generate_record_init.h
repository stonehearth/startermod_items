// boilerplate intentionally omitted...
#include "om/reflection/clear.h"

#define OM_BEGIN_CLASS(Cls, cls, Base)
#define OM_END_CLASS(Cls, cls, Base)

#define OM_PROPERTY_SET(Cls, lname, Uname, Type)    dm::Boxed<Type> lname ## _;
#define OM_PROPERTY_GET(Cls, lname, Uname, Type)    dm::Boxed<Type> lname ## _;
#define OM_PROPERTY(Cls, lname, Uname, Type)        dm::Boxed<Type> lname ## _;

