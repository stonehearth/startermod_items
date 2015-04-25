#include "radiant.h"
#include "om/components/data_store.ridl.h"
#include "om/components/data_store_ref_wrapper.h"
#include "resources/res_manager.h"
#include "lib/lua/register.h"
#include "simulation/simulation.h"
#include "client/client.h"
#include "om/components/data_store_ref_wrapper.h"

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
            boost::optional<DataStorePtr> ds = luabind::object_cast_nothrow<DataStorePtr>(saved_variables);
            if (ds) {
               (*ds)->Destroy();
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

DataStore::DataStore() :
   _isCppManaged(false)
{
   (*data_object_).SetDataObjectChanged([this]() {
      if (_weakTable.is_valid()) {
         luabind::object c = _weakTable["controller"];
         if (luabind::type(c) == LUA_TTABLE) {
            c["_sv"] = GetData();
         }
      }
   });
}

DataStore::~DataStore()
{
   DS_LOG(9) << "destroying datastore";
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

static const char* components[] = {
      "stonehearth:ai",
      "stonehearth:observers",
      "stonehearth:commands",
      "stonehearth:thought_bubble",
      "stonehearth:center_of_attention_spot",
      "stonehearth:material",
      "stonehearth:resource_node",
      "stonehearth:renewable_resource_node",
      "stonehearth:stockpile",
      "stonehearth:mining_zone",
      "stonehearth:trapping_grounds",
      "stonehearth:bait_trap",
      "stonehearth:pet",
      "stonehearth:pet_owner",
      "stonehearth:water",
      "stonehearth:waterfall",
      "stonehearth:door",
      "stonehearth:lamp",
      "stonehearth:workshop",
      "stonehearth:crafter" ,
      "stonehearth:promotion_talisman",
      "stonehearth:job",
      "stonehearth:iconic_form",
      "stonehearth:entity_forms",
      "stonehearth:ghost_form",
      "stonehearth:firepit",
      "stonehearth:lease",
      "stonehearth:lease_holder",
      "stonehearth:sensor_ai_injector" ,
      "stonehearth:posture" ,
      "stonehearth:leash" ,
      "stonehearth:equipment",
      "stonehearth:equipment_piece",
      "stonehearth:attributes",
      "stonehearth:score",
      "stonehearth:buffs",
      "stonehearth:fabricator",
      "stonehearth:fixture_fabricator",
      "stonehearth:scaffolding_fabricator",
      "stonehearth:portal",
      "stonehearth:fixture",
      "stonehearth:no_construction_zone",
      "stonehearth:construction_data",
      "stonehearth:construction_progress",
      "stonehearth:construction_project",
      "stonehearth:personality",
      "stonehearth:backpack",
      "stonehearth:carry_block",
      "stonehearth:farmer_field",
      "stonehearth:shepherd_pasture",
      "stonehearth:shepherded_animal",
      "stonehearth:growing",
      "stonehearth:crop",
      "stonehearth:dirt_plot",
      "stonehearth:target_tables",
      "stonehearth:combat_state",
      "stonehearth:building",
      "stonehearth:floor",
      "stonehearth:column",
      "stonehearth:wall",
      "stonehearth:roof",
      "stonehearth:pathfinder",
      "stonehearth:ladder",
      "stonehearth:stacks_model_renderer",
      "stonehearth:party_member",
      "stonehearth:loot_drops",
};

// This is a REALLY crappy way to do this.  We should store this bit when we save,
// but that would break compatibility with Alpha 9.  Do this for now, and remove it
// when porting to develop
bool DataStore::GetIsCppManaged() const
{
   for (int i = 0; i < ARRAY_SIZE(components); i++) {
      if (strcmp(components[i], (*controller_name_).c_str()) == 0) {
         return true;
      }
   }
   return false;
}

void DataStore::RemoveKeepAliveReferences()
{
   if (_isCppManaged) {
      DS_LOG(9) << "preserving keep alive reference to controller of cpp managed datastore";
      return;
   }
   DS_LOG(9) << "clearing keep alive reference";
   _controllerKeepAliveObject = luabind::object();
}

void DataStore::RestoreController(DataStorePtr self, bool isCppManaged)
{
   if (_weakTable.is_valid()) {
      return;
   }
   _isCppManaged = isCppManaged || GetIsCppManaged();

   lua_State* L = GetData().interpreter();
   lua::ScriptHost *scriptHost = lua::ScriptHost::GetScriptHost(L);
   std::string uri = GetControllerUri();

   if (!uri.empty()) {
      try {
         luabind::object ctor = scriptHost->RequireScript(uri);
         if (ctor) {
            luabind::object controller = ctor();
            if (controller) {
               luabind::object saved_variables;

               // this keeps the controller alive until some external reference
               // gets a chance to point to it.
               _controllerKeepAliveObject = controller;

               if (_isCppManaged) {
                  DS_LOG(9) << "restoring controller " << uri << " (weak)";
                  saved_variables = luabind::object(L, om::DataStoreRefWrapper(self));
               } else {
                  DS_LOG(9) << "restoring controller " << uri << " (strong)";
                  saved_variables = luabind::object(L, self);
               }
               SetController(controller);
               controller["__saved_variables"] = saved_variables;
               controller["_sv"] = GetData();
               PatchLuaDestructor();
            }
         }
         if (_weakTable.is_valid()) {
            DS_LOG(9) << "after creating controller, weak value is " << (luabind::object(_weakTable["controller"]).is_valid() ? luabind::type(_weakTable["controller"]) : -1);
         }
      } catch (std::exception const& e) {
         lua::ScriptHost::ReportCStackException(L, e);
      }
   }
}

// Monkey patch destroy to automatically destroy the datastore
void DataStore::PatchLuaDestructor()
{
   ASSERT(_weakTable);
   ASSERT(!_controllerDestructor);

   lua_State* L = _weakTable.interpreter();
   luabind::object destroy_fn;

   // Check the controller metatable.
   luabind::object controller = _weakTable["controller"];

   luabind::object hook_fn = lua::GetPointerToCFunction(L, &LuaDestructorHook);
   luabind::object mt = luabind::getmetatable(controller);
   if (mt) {
      destroy_fn = mt["destroy"];
      if (luabind::type(destroy_fn) == LUA_TFUNCTION) {
         ASSERT(destroy_fn != hook_fn);
         _controllerDestructor = destroy_fn;
      }
   }

   // Check the controller itself.
   if (!_controllerDestructor) {
      destroy_fn = controller["destroy"];
      if (luabind::type(destroy_fn) == LUA_TFUNCTION) {
         ASSERT(destroy_fn != hook_fn);
         _controllerDestructor = destroy_fn;
      }
   }
   controller["destroy"] = hook_fn;
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

      om::DataStorePtr dsObj;
      boost::optional<om::DataStorePtr> ds = luabind::object_cast_nothrow<om::DataStorePtr>(*i);
      if (ds) {
         dsObj = ds.get();
      } else {
         boost::optional<om::DataStoreRefWrapper> dr = luabind::object_cast_nothrow<om::DataStoreRefWrapper>(*i);
         if (dr) {
            dsObj = dr.get().lock();
         }
      }
      if (!dsObj) {
         DS_LOG(7) << "table key '" << key << "' is not a datastore.  ignoring";
         continue;         
      }
      if (!dsObj) {
         DS_LOG(7) << "table key '" << key << "' an orphended datastore.  ignoring";
         continue;
      }
      luabind::object controller = dsObj->GetController();
      if (!controller) {
         DS_LOG(7) << "table key '" << key << "' datastore " << dsObj->GetObjectId() << " has no controller.  ignoring";
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
   if (!_weakTable.is_valid()) {
      DS_LOG(9) << "weak table is invalid in GetController().  returning LUA_TNIL";
      return luabind::object();
   }
   luabind::object controller = _weakTable["controller"];
   DS_LOG(9) << "keep alive controller is " << (_controllerKeepAliveObject.is_valid() ? luabind::type(_controllerKeepAliveObject) : -1) << " in GetController()";
   DS_LOG(9) << "weak value is " << (luabind::object(_weakTable["controller"]).is_valid() ? luabind::type(_weakTable["controller"]) : -1);
   DS_LOG(9) << "returning controller " << (controller.is_valid() ? luabind::type(controller) : -1 )<< " in GetController()";
   return controller;
}

void DataStore::CallLuaDestructor()
{
   DS_LOG(3) << " calling lua destructor";

   luabind::object controller = GetController();
   if (controller && _controllerDestructor) {
      lua_State* cb_thread = lua::ScriptHost::GetCallbackThread(_controllerDestructor.interpreter());  
      try {
         luabind::object destructor(cb_thread, _controllerDestructor);
         destructor(controller);
      } catch (std::exception const& e) {
         lua::ScriptHost::ReportCStackException(cb_thread, e);
      }
   }
}

void DataStore::Destroy()
{
   if (_weakTable) {
      lua_State* L = _weakTable.interpreter();
      ASSERT(L);
      CallLuaDestructor();
   }
}

void DataStore::SetController(luabind::object controller)
{
   ASSERT(controller.is_valid());

   // Stick our reference to our own controller in a weak table to avoid cycles
   // (our controller may be keeping a reference on us in __saved_variables!)
   if (!_weakTable.is_valid()) {
      lua_State* L = controller.interpreter();
      _weakTable = luabind::newtable(L);
      if (strcmp(log::GetCurrentThreadName(), "server") == 0) {
         luabind::object metatable = luabind::newtable(L);
         metatable["__mode"] = "v";
         luabind::setmetatable(_weakTable, metatable);
         DS_LOG(9) << "creating weak table.";
      }
   }
   _weakTable["controller"] = controller;
   DS_LOG(9) << "in set controller, controller value is " 
             << (controller.is_valid() ? luabind::type(controller) : -1) 
             << ", weak value is " 
             << (luabind::object(_weakTable["controller"]).is_valid() ? luabind::type(_weakTable["controller"]) : -1);
}

luabind::object DataStore::CreateController(DataStorePtr self, std::string const& type, std::string const& name)
{
   controller_type_ = type;
   controller_name_ = name;

   // Ordering is important here to make sure the controller doesn't get
   // reaped by the GC before we can return a reference to it.
   RestoreController(self);
   luabind::object controller = GetController();
   RemoveKeepAliveReferences();

   return controller;
}

void DataStore::OnLoadObject(dm::SerializationType r)
{
}

dm::Boxed<lua::DataObject> const& DataStore::GetDataObjectBox() const
{
   return data_object_;
}
