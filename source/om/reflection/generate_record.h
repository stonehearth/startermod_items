// boilerplate intentionally omitted...
#include "om/reflection/clear.h"

#define OM_BEGIN_CLASS(Cls, cls, Base)       \
   void InitializeRecordFields() override    \
   {                                         \
      Component::InitializeRecordFields();

#define OM_END_CLASS(Cls, cls, Base)         \
      if (!IsRemoteRecord()) {               \
         ConstructObject();                  \
      }                                      \
   }

#define OM_PROPERTY_SET(Cls, lname, Uname, Type)    AddRecordField(#lname, lname ## _);
#define OM_PROPERTY_GET(Cls, lname, Uname, Type)    AddRecordField(#lname, lname ## _);
#define OM_PROPERTY(Cls, lname, Uname, Type)        AddRecordField(#lname, lname ## _);

