#ifndef _RADIANT_OM_DATA_BINDING_H
#define _RADIANT_OM_DATA_BINDING_H

#include "dm/object.h"
#include "dm/record.h"
#include "dm/store.h"
#include "dm/set.h"
#include "dm/map.h"
#include "dm/boxed.h"
#include "om/object_enums.h"
#include "om/om.h"
#include "om/entity.h"
#include "csg/cube.h"
#include "lib/lua/bind.h"

BEGIN_RADIANT_OM_NAMESPACE

class DataBindingValue
{
public:
   luabind::object GetDataObject() const;
   luabind::object GetModelObject() const;
   JSONNode GetJsonData() const;

   void SetDataObject(luabind::object data);
   void SetModelObject(luabind::object data);
 
public: // xxx: remove during the load / save refactor
   void Save(const dm::Store& store, Protocol::Value* msg) const;
   void Load(const dm::Store& store, const Protocol::Value& msg);

private:
   luabind::object            data_;
   luabind::object            model_;
   mutable JSONNode           cached_json_;
   mutable dm::GenerationId   last_encode_;
};

class DataBinding : public dm::Boxed<DataBindingValue>
{
};

// Create a new type for luabind.
class DataBindingP : public DataBinding
{
};


std::ostream& operator<<(std::ostream& os, DataBinding const& o);

END_RADIANT_OM_NAMESPACE

// xxx: move this in the loader refactor
BEGIN_RADIANT_NAMESPACE

template<>
struct dm::SaveImpl<om::DataBindingValue> {
   static void SaveValue(const dm::Store& store, Protocol::Value* msg, om::DataBindingValue const& obj) {
      obj.Save(store, msg);
   }
   static void LoadValue(const Store& store, const Protocol::Value& msg, om::DataBindingValue& obj) {
      obj.Load(store, msg);
   }
   static void GetDbgInfo(om::DataBindingValue const& obj, dm::DbgInfo &info) {
      info.os << "[data_binding_value " << obj.GetJsonData().write() << "]";
   }
};

END_RADIANT_NAMESPACE

#endif //  _RADIANT_OM_DATA_BINDING_H
