#include "radiant.h"
#include "object_formatter.h"
#include <regex>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

// templates...
#include "dm/store.h"
#include "om/om.h"
#include "dm/store.h"
#include "om/all_objects.h"
#include "om/all_components.h"
#include "lib/json/node.h"
#include "lib/json/om_json.h"

using namespace ::radiant;
using namespace ::radiant::om;

// xxx - It would be great if we could do this with a C++ 11 initializer, but VC doesn't
// support those yet.
static std::unordered_map<dm::ObjectType, std::function<json::Node(const ObjectFormatter& f, dm::ObjectPtr)>> object_formatters_;
static void CreateDispatchTable();

JSONNode ObjectFormatter::ObjectToJson(dm::ObjectPtr obj) const
{
   CreateDispatchTable();

   if (obj) {
      auto i = object_formatters_.find(obj->GetObjectType());
      if (i != object_formatters_.end()) {
         JSONNode result = i->second(*this, obj);
         if (result.type() == JSON_NODE) {
            result.push_back(JSONNode("__self", obj->GetStoreAddress()));
         }
         return result;
      }
   }
   // xxx: throw an exception...
   return JSONNode();
}

static void CreateDispatchTable()
{
   if (object_formatters_.empty()) {

#define OM_OBJECT(Cls, lower)                                       \
   object_formatters_[om::Cls ## ObjectType] =                      \
      [](const ObjectFormatter& f, dm::ObjectPtr obj) -> json::Node { \
         return json::encode(static_cast<Cls&>(*obj.get()));        \
      };
   OM_ALL_OBJECTS
   OM_ALL_COMPONENTS
   OM_OBJECT(JsonBoxed, json_boxed)
#undef OM_OBJECT

   }
}
