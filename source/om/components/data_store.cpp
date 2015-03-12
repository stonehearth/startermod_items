#include "radiant.h"
#include "om/components/data_store.ridl.h"
#include "resources/res_manager.h"
#include "lib/lua/register.h"
#include "simulation/simulation.h"
#include "client/client.h"

using namespace ::radiant;
using namespace ::radiant::om;

#define DS_LOG(level)      LOG(om.data_store, level) << "[datastore id:" << GetObjectId() << " ctrl:" << *controller_name_ << "] "

static bool IsServer()
{
   if (strcmp(log::GetCurrentThreadName(), "server") == 0) {
      return true;
   }
   ASSERT(strcmp(log::GetCurrentThreadName(), "client") == 0);
   return false;
}


int DataStore::LuaDestructorHook(lua_State* L) {   
   try {
      luabind::object self = luabind::object(luabind::from_stack(L, 1));
      if (luabind::type(self) == LUA_TTABLE) {
         luabind::object saved_variables = self["__saved_variables"];
         if (luabind::type(saved_variables) == LUA_TUSERDATA) {
            boost::optional<DataStoreRef> o = luabind::object_cast_nothrow<DataStoreRef>(saved_variables);
            if (o) {
               DataStorePtr ds = o->lock();
               if (ds) {
                  ds->Destroy();
               }
            }
         }
      }
   } catch (std::exception const& e) {
      lua::ScriptHost::ReportCStackException(L, e);
   }
   return 0;
}

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
   node.set("__controller", *controller_name_);
}

void DataStore::RestoreController(DataStoreRef self)
{
   if (_controllerObject.is_valid()) {
      return;
   }

   lua_State* L = GetData().interpreter();
   lua::ScriptHost *scriptHost = lua::ScriptHost::GetScriptHost(L);
   std::string uri = GetControllerUri();

   if (!uri.empty()) {
      try {
         luabind::object ctor = scriptHost->RequireScript(uri);
         if (ctor) {
            DS_LOG(3) << "restored controller for script " << uri;
            _controllerObject = ctor();
            if (_controllerObject) {
               _controllerObject["__saved_variables"] = self;
               _controllerObject["_sv"] = GetData();
               PatchLuaDestructor();
            }
         }
      } catch (std::exception const& e) {
         lua::ScriptHost::ReportCStackException(L, e);
      }
   }

   // Whenever the data object changes, write the root value back into
   // _sv of the controller.  This won't help anyone who caches _sv values
   // in their code (e.g. local foo = self._sv.foo), but it at least makes
   // it so client-side code doesn't have to repeatedly call :get()
   // on the datastore to make sure they have the most up-to-date copy

   _dataObjTrace = data_object_.TraceChanges("update _sv", dm::OBJECT_MODEL_TRACES)
                              ->OnModified([this]() {
                                 if (_controllerObject.is_valid()) {
                                    _controllerObject["_sv"] = GetData();
                                 }
                              });
}

// Monkey patch destroy to automatically destroy the datastore
void DataStore::PatchLuaDestructor()
{
   ASSERT(_controllerObject);
   ASSERT(!_controllerDestructor);

   lua_State* L = _controllerObject.interpreter();
   luabind::object destroy_fn;

   // Check the controller metatable.
   luabind::object hook_fn = lua::GetPointerToCFunction(L, &LuaDestructorHook);
   luabind::object mt = luabind::getmetatable(_controllerObject);
   if (mt) {
      destroy_fn = mt["destroy"];
      if (luabind::type(destroy_fn) == LUA_TFUNCTION) {
         ASSERT(destroy_fn != hook_fn);
         _controllerDestructor = destroy_fn;
      }
   }

   // Check the controller itself.
   if (!_controllerDestructor) {
      destroy_fn = _controllerObject["destroy"];
      if (luabind::type(destroy_fn) == LUA_TFUNCTION) {
         ASSERT(destroy_fn != hook_fn);
         _controllerDestructor = destroy_fn;
      }
   }
   _controllerObject["destroy"] = hook_fn;
}

void DataStore::RestoreControllerData()
{
   if ((*data_object_).GetNeedsRestoration()) {
      DS_LOG(7) << "begin restore controller data...";
      (*data_object_).SetNeedsRestoration(false);
      luabind::object data = GetData();
      luabind::object visited = luabind::newtable(data.interpreter());
      RestoreControllerDataRecursive(GetData(), visited);
      DS_LOG(7) << "... end restore controller.";
   }
}

void DataStore::RestoreControllerDataRecursive(luabind::object o, luabind::object& visited)
{
   if (luabind::type(o) != LUA_TTABLE) {
      DS_LOG(7) << "object is not a table.  skipping.";
      return;
   }

   if (luabind::type(visited[o]) != LUA_TNIL) {
      DS_LOG(5) << "already visited lua object while restoring datastore.  bailing";
      return;
   }
   visited[o] = true;

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
         RestoreControllerDataRecursive(*i, visited);
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

      if (!IsServer()) {
         type = "client_" + type;
      }
      std::string result;
      res::ResourceManager2::GetInstance().LookupManifest(modname, [&result, &type, &compName](const res::Manifest& manifest) {
         result = manifest.get_node(type).get<std::string>(compName, "");
      });
      return result;
   }
   // assume the name is the direct path to the script
   return name;
}

luabind::object DataStore::GetController() const
{
   return _controllerObject;
}

void DataStore::CallLuaDestructor()
{
   DS_LOG(3) << " calling lua destructor";

   if (_controllerObject && _controllerDestructor) {
      lua_State* cb_thread = lua::ScriptHost::GetCallbackThread(_controllerDestructor.interpreter());  
      try {
         luabind::object destructor(cb_thread, _controllerDestructor);
         destructor(_controllerObject);
      } catch (std::exception const& e) {
         lua::ScriptHost::ReportCStackException(cb_thread, e);
      }
      _controllerDestructor = luabind::object();
      _controllerObject = luabind::object();
   }
}

void DataStore::Destroy()
{
   if (_controllerObject) {
      lua_State* L = _controllerObject.interpreter();
      ASSERT(L);
      CallLuaDestructor();
   }

   dm::ObjectId id = GetObjectId();
   if (IsServer()) {
      simulation::Simulation::GetInstance().RemoveDataStoreFromMap(id);
   } else {
      client::Client::GetInstance().RemoveDataStoreFromMap(id);
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
