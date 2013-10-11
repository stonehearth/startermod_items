#ifndef _RADIANT_OM_JSON_STORE_H
#define _RADIANT_OM_JSON_STORE_H

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

class JsonStore : public dm::Object
{
public:
   DEFINE_OM_OBJECT_TYPE(JsonStore, json_store);
   void GetDbgInfo(dm::DbgInfo &info) const override;

   JSONNode& Modify();
   JSONNode const& Get() const;
   
protected:
   void SaveValue(const dm::Store& store, Protocol::Value* msg) const override;
   void LoadValue(const dm::Store& store, const Protocol::Value& msg) override;

private:
   JSONNode       json_;
};

std::ostream& operator<<(std::ostream& os, const JsonStore& o);

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_JSON_STORE_H
