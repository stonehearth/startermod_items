// boilerplate intentionally omitted...
#include "om/reflection/clear.h"

#define OM_BEGIN_CLASS(Cls, cls, Base)  \
   lua::RegisterWeakGameObjectDerived<Cls, Base>() \
      .def("get_entity",     &Base::GetEntityRef) \
      .def("extend",         &Base::ExtendObject)
 
#define OM_END_CLASS(Cls, cls, Base)    \
   ;

#define OM_PROPERTY_SET(Cls, lname, Uname, Type)    .def("set_" #lname, &PropertySet<Type, Cls, &Cls::Set ## Uname>)
#define OM_PROPERTY_GET(Cls, lname, Uname, Type)    .def("get_" #lname, &PropertyGet<Type, Cls, &Cls::Get ## Uname>)
#define OM_PROPERTY(Cls, lname, Uname, Type)    \
   OM_PROPERTY_SET(Cls, lname, Uname, Type)     \
   OM_PROPERTY_GET(Cls, lname, Uname, Type)
