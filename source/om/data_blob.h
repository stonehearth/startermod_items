#ifndef _RADIANT_OM_DATA_BLOB_H
#define _RADIANT_OM_DATA_BLOB_H

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

class DataBlob : public dm::Object
{
public:
   DataBlob();
   DEFINE_OM_OBJECT_TYPE_NO_CONS(DataBlob, data_blob);

   void SetLuaObject(luabind::object obj);
   luabind::object GetLuaObject() const { return obj_; }

   JSONNode ToJson() const;

#if 0
   dm::Guard Trace(const char* reason, std::function<void(JSONNode const &)> cb) const {
      return TraceObjectChanges(reason, [=]() {
         cb(json_);
      });
   }
#endif

protected:
   void SaveValue(const dm::Store& store, Protocol::Value* msg) const override;
   void LoadValue(const dm::Store& store, const Protocol::Value& msg) override;

private:
   luabind::object      obj_;
   mutable JSONNode     cached_json_;
   mutable bool         cached_json_valid_;
};

std::ostream& operator<<(std::ostream& os, const DataBlob& o);

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_DATA_BLOB_H
