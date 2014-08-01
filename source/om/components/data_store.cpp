#include "pch.h"
#include "om/components/data_store.ridl.h"
#include "resources/res_manager.h"

using namespace ::radiant;
using namespace ::radiant::om;

#define DS_LOG(level)      LOG(om.data_store, level)

std::ostream& operator<<(std::ostream& os, DataStore const& o)
{
   return (os << "[DataStore]");
}

DataStore::DataStore()
{
}

DataStore::~DataStore()
{
}

luabind::object DataStore::GetData() const
{
   return (*data_object_).GetDataObject();
}

void DataStore::SetData(luabind::object o)
{
   data_object_.Modify([o](lua::DataObject& obj) {
      obj.SetDataObject(o);
   });
}

void DataStore::MarkDataChanged()
{
   data_object_.Modify([](lua::DataObject& obj) {
      obj.MarkDirty();
   });
}

void DataStore::LoadFromJson(json::Node const& obj)
{
   NOT_YET_IMPLEMENTED(); // and probably shouldn't be...
}

void DataStore::SerializeToJson(json::Node& node) const
{
   node = (*data_object_).GetJsonNode();
   Record::SerializeToJson(node);
}

luabind::object DataStore::RestoreController(DataStorePtr self)
{
   // Whenever the data object changes, write the root value back into
   // _sv of the controller.  This won't help anyone who caches _sv values
   // in their code (e.g. local foo = self._sv.foo), but it at least makes
   // it so client-side code doesn't have to repeatedly call :get_data()
   // on the datastore to make sure they have the most up-to-date copy

   _dataObjTrace = data_object_.TraceChanges("update _sv", dm::OBJECT_MODEL_TRACES)
                              ->OnModified([this]() {
                                 luabind::object controller = GetController();
                                 if (controller.is_valid()) {
                                    _controllerObject["_sv"] = GetData();
                                 }
                              });

   // Now go ahead and create the controller
   std::vector<luabind::object> visitedTables;
   std::unordered_map<dm::ObjectId, luabind::object> visited;
   return RestoreControllerRecursive(self, visitedTables, visited);
}

luabind::object DataStore::RestoreControllerRecursive(DataStorePtr self, std::vector<luabind::object> visitedTables, std::unordered_map<dm::ObjectId, luabind::object>& visited)
{
   ASSERT(visited.find(GetObjectId()) == visited.end());

   // Stick an invalid entry in the map of already loaded controllers
   // to prevent an infinite loops restoring DataStores with graph
   // like structures.
   visited[GetObjectId()] = luabind::object();

   // Make sure we dont...
   if ((*data_object_).GetNeedsRestoration()) {

      (*data_object_).SetNeedsRestoration(false);

      if (!_controllerObject.is_valid()) {
         lua_State* L = GetData().interpreter();
         lua::ScriptHost *scriptHost = lua::ScriptHost::GetScriptHost(L);

         std::string uri = GetControllerUri();
         if (!uri.empty()) {
            try {
               luabind::object ctor = scriptHost->RequireScript(L, uri);
               if (ctor) {
                  _controllerObject = ctor();
                  if (_controllerObject) {
                     _controllerObject["__saved_variables"] = self;
                     _controllerObject["_sv"] = GetData();
                  }
               }
            } catch (std::exception const& e) {

               lua::ScriptHost::ReportCStackException(L, e);
            }
         }
      }

      // Now that we have set and constructed the controller
      // object, we can restore all the contained datastores.  This
      // way, if an object in our datastore loops back and points to
      // us, we can return our controller instead of entering an
      // infinite loop!
      visited[GetObjectId()] = _controllerObject;
      RestoreContainedDatastores(GetData(), visitedTables, visited);
   }
   return _controllerObject;
}

void DataStore::RestoreContainedDatastores(luabind::object o, std::vector<luabind::object> visitedTables, std::unordered_map<dm::ObjectId, luabind::object>& visited)
{
   if (luabind::type(o) == LUA_TTABLE) {
      for (luabind::object v : visitedTables) {
         if (v == o) {
            DS_LOG(8) << "already visited lua object while restoring datastore.  bailing";
            return;
         }
      }
      visitedTables.emplace_back(o);

      for (luabind::iterator i(o), end; i != end; i++) {
         int t = luabind::type(i.key());
         std::string key;
         if (LOG_IS_ENABLED(om.data_store, 8)) {
            if (t == LUA_TSTRING) { 
               key = luabind::object_cast<std::string>(i.key());
            } else {
               key = BUILD_STRING("type(" << t << ")");
            }
         }

         if (luabind::type(*i) == LUA_TTABLE) {
            DS_LOG(8) << "restoring table for key " << key;
            RestoreContainedDatastores(*i, visitedTables, visited);
         } else {
            boost::optional<om::DataStorePtr> ds = luabind::object_cast_nothrow<om::DataStorePtr>(*i);
            if (ds) {
               luabind::object controller;
               om::DataStorePtr datastore = ds.get();
               dm::ObjectId id = datastore->GetObjectId();

               auto j = visited.find(id);
               if (j != visited.end()) {
                  DS_LOG(8) << "already visisted datastore at " << key << ".  using cached controller";
                  controller = j->second;
               } else {
                  DS_LOG(8) << "restoring datastore at " << key << ".";
                  controller = datastore->RestoreControllerRecursive(datastore, visitedTables, visited);
               }
               if (controller.is_valid()) {
                  if (luabind::type(i.key()) == LUA_TSTRING) {
                     DS_LOG(9) << "restoring '" << key << "' datastore";
                  }
                  *i = controller;
               }
            }
         }
      }
   }
}

std::string
DataStore::GetControllerUri()
{
   std::string type = *controller_type_;
   std::string const& name = *controller_name_;

   std::string modname;
   size_t offset = name.find(':');

   if (offset != std::string::npos) {
      lua_State* L = GetData().interpreter();

      modname = name.substr(0, offset);
      std::string compName = name.substr(offset + 1, std::string::npos);

      bool isServer = luabind::object_cast<bool>(luabind::globals(L)["radiant"]["is_server"]);
      if (!isServer) {
         type = "client_" + type;
      }
      std::string result;
      res::ResourceManager2::GetInstance().LookupManifest(modname, [&result, &type, &compName](const res::Manifest& manifest) {
         result = manifest.get_node(type).get<std::string>(compName, "");
      });
      return result;
   }
   // xxx: throw an exception...
   return "";
}

luabind::object DataStore::GetController() const
{
   return _controllerObject;
}

void DataStore::DestroyController()
{
   if (_controllerObject) {
      lua_State* L = lua::ScriptHost::GetCallbackThread(_controllerObject.interpreter());
      try {
         luabind::object destroy = _controllerObject["destroy"];
         if (destroy) {
            luabind::object cb(L, destroy);
            cb(_controllerObject);
         }
      } catch (std::exception const& e) {
         lua::ScriptHost::ReportCStackException(L, e);
      }
   }
}

void DataStore::SetController(luabind::object controller)
{
   ASSERT(controller.is_valid());
   _controllerObject = controller;
}

luabind::object DataStore::CreateController(DataStorePtr self, std::string type, std::string const& name)
{
   controller_type_ = type;
   controller_name_ = name;

   return RestoreController(self);
}

void DataStore::OnLoadObject(dm::SerializationType r)
{
}

dm::Boxed<lua::DataObject> const& DataStore::GetDataObjectBox() const
{
   return data_object_;
}
