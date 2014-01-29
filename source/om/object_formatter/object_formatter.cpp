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

static const std::regex path_regex__("/o/stores/([^/]*)/objects/(\\d+)");

// xxx - It would be great if we could do this with a C++ 11 initializer, but VC doesn't
// support those yet.
static std::unordered_map<dm::ObjectType, std::function<json::Node(const ObjectFormatter& f, dm::ObjectPtr)>> object_formatters_;
static void CreateDispatchTable();

template <typename T> JSONNode ToJson(const ObjectFormatter& f, T const& obj);

ObjectFormatter::ObjectFormatter()
{
}

JSONNode ObjectFormatter::ObjectToJson(dm::ObjectPtr obj) const
{
   CreateDispatchTable();

   if (obj) {
      auto i = object_formatters_.find(obj->GetObjectType());
      if (i != object_formatters_.end()) {
         JSONNode result = i->second(*this, obj);
         if (result.type() == JSON_NODE) {
            result.push_back(JSONNode("__self", GetPathToObject(obj)));
         }
         return result;
      }
   }
   // xxx: throw an exception...
   return JSONNode();
}

bool ObjectFormatter::IsPathInStore(dm::Store const& store, std::string const& path) const
{
   std::smatch match;
   if (std::regex_match(path, match, path_regex__)) {
      std::string store_name = match[1].str();
      return store.GetName() == store_name;
   }
   return false;
}

dm::ObjectPtr ObjectFormatter::GetObject(dm::Store const& store, std::string const& path) const
{
   std::smatch match;
   if (std::regex_match(path, match, path_regex__)) {
      std::string store_name = match[1].str();
      if (store.GetName() == store_name) {
         std::string object_id = match[2].str();
         return store.FetchObject<dm::Object>(atoi(object_id.c_str()));
      }
   }
   return nullptr;
}

std::string ObjectFormatter::GetPathToObject(dm::ObjectPtr obj) const
{
   if (obj) {
      std::ostringstream output;
      output << "/o/stores/" << obj->GetStore().GetName() << "/objects/" << obj->GetObjectId();
      return output.str();
   }
   return "null";
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
