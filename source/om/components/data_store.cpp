#include "radiant.h"
#include "om/components/data_store.ridl.h"
#include "resources/res_manager.h"

using namespace ::radiant;
using namespace ::radiant::om;

#define DS_LOG(level)      LOG(om.data_store, level) << "[datastore id:" << GetObjectId() << "] "

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
   DS_LOG(9) << "marking data object id:" << data_object_.GetObjectId() << " changed";
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

void DataStore::RestoreController(DataStoreRef self)
{
   if (!_controllerObject.is_valid() || (*data_object_).GetNeedsRestoration()) {
      lua_State* L = GetData().interpreter();
      lua::ScriptHost *scriptHost = lua::ScriptHost::GetScriptHost(L);
      std::string uri = GetControllerUri();

      (*data_object_).SetNeedsRestoration(false);

      if (!uri.empty()) {
         try {
            luabind::object ctor = scriptHost->RequireScript(L, uri);
            if (ctor) {
               DS_LOG(3) << "restored controller for script " << uri;
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

   // Whenever the data object changes, write the root value back into
   // _sv of the controller.  This won't help anyone who caches _sv values
   // in their code (e.g. local foo = self._sv.foo), but it at least makes
   // it so client-side code doesn't have to repeatedly call :get_data()
   // on the datastore to make sure they have the most up-to-date copy

   _dataObjTrace = data_object_.TraceChanges("update _sv", dm::OBJECT_MODEL_TRACES)
                              ->OnModified([this]() {
                                 if (_controllerObject.is_valid()) {
                                    _controllerObject["_sv"] = GetData();
                                 }
                              });
}

void DataStore::RestoreControllerData(std::vector<luabind::object>& visitedTables)
{
   DS_LOG(7) << "begin restore controller data...";
   RestoreControllerDataRecursive(GetData(), visitedTables);
   DS_LOG(7) << "... end restore controller.";
}

void DataStore::RestoreControllerDataRecursive(luabind::object o, std::vector<luabind::object>& visitedTables)
{
   if (luabind::type(o) != LUA_TTABLE) {
      DS_LOG(7) << "object is not a table.  skipping.";
      return;
   }

   for (luabind::object v : visitedTables) {
      if (v == o) {
         DS_LOG(5) << "already visited lua object while restoring datastore.  bailing";
         return;
      }
   }
   visitedTables.emplace_back(o);

   for (luabind::iterator i(o), end; i != end; ++i) {
      int t = luabind::type(i.key());
      std::string key;
      if (LOG_IS_ENABLED(om.data_store, 5)) {
         if (t == LUA_TSTRING) { 
            key = luabind::object_cast<std::string>(i.key());
         } else if (t == LUA_TNUMBER) { 
            key = BUILD_STRING(luabind::object_cast<double>(i.key()));
         } else {
            key = BUILD_STRING("type(" << t << ")");
         }
      }

      if (luabind::type(*i) == LUA_TTABLE) {
         DS_LOG(5) << "restoring table for key " << key;
         RestoreControllerDataRecursive(*i, visitedTables);
         continue;
      }

      boost::optional<om::DataStoreRef> ds = luabind::object_cast_nothrow<om::DataStoreRef>(*i);
      if (!ds) {
         DS_LOG(7) << "table key '" << key << "' is not a datastore.  ignoring";
         continue;
      }
      om::DataStoreRef datastore = ds.get();
      om::DataStorePtr dsObj = datastore.lock();
      if (!dsObj) {
         DS_LOG(7) << "table key '" << key << "' an orphended datastore.  ignoring";
         continue;
      }
      luabind::object controller = dsObj->GetController();
      if (!controller) {
         DS_LOG(7) << "table key '" << key << "' datastore has no controller.  ignoring";
         continue;
      }
      DS_LOG(7) << "restoring '" << key << "' datastore";
      *i = controller;
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

luabind::object DataStore::CreateController(DataStoreRef self, std::string const& type, std::string const& name)
{
   controller_type_ = type;
   controller_name_ = name;

   RestoreController(self);
   return _controllerObject;
}

void DataStore::OnLoadObject(dm::SerializationType r)
{
}

dm::Boxed<lua::DataObject> const& DataStore::GetDataObjectBox() const
{
   return data_object_;
}
