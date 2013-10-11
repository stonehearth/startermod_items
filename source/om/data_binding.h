#ifndef _RADIANT_OM_DATA_BINDING_H
#define _RADIANT_OM_DATA_BINDING_H

#include "dm/object.h"
#include "dm/record.h"
#include "dm/store.h"
#include "dm/set.h"
#include "dm/map.h"
#include "om/object_enums.h"
#include "om/om.h"
#include "om/entity.h"
#include "csg/cube.h"

BEGIN_RADIANT_OM_NAMESPACE

class DataBinding : public dm::Object
{
public:
   DataBinding();
   DEFINE_OM_OBJECT_TYPE_NO_CONS(DataBinding, data_binding);
   void GetDbgInfo(dm::DbgInfo &info) const override;


   luabind::object GetDataObject() const;
   luabind::object GetModelObject() const;
   JSONNode GetJsonData() const;

   void SetDataObject(luabind::object data);
   void SetModelObject(luabind::object data);
 
protected:
   void SaveValue(const dm::Store& store, Protocol::Value* msg) const override;
   void LoadValue(const dm::Store& store, const Protocol::Value& msg) override;

private:
   luabind::object            data_;
   luabind::object            model_;
   mutable JSONNode           cached_json_;
   mutable dm::GenerationId   last_encode_;
};

// Create a new type for luabuild.
class DataBindingP : public DataBinding
{
public:
   DataBindingP() : DataBinding() { }
   DEFINE_OM_OBJECT_TYPE_NO_CONS(DataBindingP, data_binding_p);
};


std::ostream& operator<<(std::ostream& os, DataBinding const& o);

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_DATA_BINDING_H
