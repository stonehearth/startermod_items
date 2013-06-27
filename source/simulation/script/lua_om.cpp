#include "pch.h"
#include "lua_om.h"
#include "om/entity.h"
#include "script_host.h"
#define DEFINE_ALL_COMPONENTS
#include "om/all_components.h"
#include "helpers.h"
#include "resources/res_manager.h"

using namespace ::radiant;
using namespace ::radiant::simulation;
using namespace ::luabind;

om::TargetTableEntryRef TargetTableEntrySetValue(om::TargetTableEntryPtr in, int value) 
{ 
   in->SetValue(value);
   return in;
}

#define OM_OBJECT(Clas, lower) \
   std::ostream& operator<<(std::ostream& out, const om::Clas& e) \
   { \
      out << "[" << om::GetObjectName(e) << " " << e.GetObjectId() << "]"; \
      return out; \
   }

OM_ALL_OBJECTS
OM_ALL_COMPONENTS
OM_OBJECT(BuildOrder,         build_order)
OM_OBJECT(GridBuildOrder,     grid_build_order)
OM_OBJECT(RegionBuildOrder,   region_build_order)

#undef OM_OBJECT

// should this be in the dm?
template <class T>
class MapPromise
{
public:
   MapPromise(const T& c) {
      auto changed = std::bind(&MapPromise::OnMapChange, this, std::placeholders::_1, std::placeholders::_2);
      auto removed = std::bind(&MapPromise::OnMapRemove, this, std::placeholders::_1);

      trace_ = c.TraceMapChanges("map key promise", changed, removed);
   }

   ~MapPromise() {
      LOG(WARNING) << "killing map key...";
   }

public:
   MapPromise* AddedPromise(object cb) {
      changedCbs_.push_back(cb);
      return this;
   }

   MapPromise* RemovedPromise(object cb) {
      removedCbs_.push_back(cb);
      return this;
   }

   void OnMapChange(const typename T::KeyType& key, const typename T::ValueType& value) {
      for (auto& cb : changedCbs_) {
         luabind::call_function<void>(cb, key, value);
      }
   }
   void OnMapRemove(const typename T::KeyType& key)  {
      for (auto& cb : removedCbs_) {
         luabind::call_function<void>(cb, key);
      }
   }

private:
   dm::Guard               trace_;
   std::vector<object>     changedCbs_;
   std::vector<object>     removedCbs_;
};

// should this be in the dm?
template <class T>
class SetPromise
{
public:
   SetPromise(const T& c) {
      auto changed = std::bind(&SetPromise::OnSetChange, this, std::placeholders::_1);
      auto removed = std::bind(&SetPromise::OnSetRemove, this, std::placeholders::_1);

      trace_ = c.TraceSetChanges("set promise", changed, removed);
   }

   ~SetPromise() {
      LOG(WARNING) << "killing SetPromise...";
   }

public:
   SetPromise* AddedPromise(object cb) {
      changedCbs_.push_back(cb);
      return this;
   }

   SetPromise* RemovedPromise(object cb) {
      removedCbs_.push_back(cb);
      return this;
   }

   void OnSetChange(const typename T::ValueType& value) {
      for (auto& cb : changedCbs_) {
         luabind::call_function<void>(cb, value);
      }
   }
   void OnSetRemove(const typename T::ValueType& value)  {
      for (auto& cb : removedCbs_) {
         luabind::call_function<void>(cb, value);
      }
   }

private:
   dm::Guard               trace_;
   std::vector<object>     changedCbs_;
   std::vector<object>     removedCbs_;
};

// should this be in the dm?
template <class T>
class BoxedPromise
{
public:
public:
   BoxedPromise(const T& c) {
      auto changed = std::bind(&BoxedPromise::OnChange, this, std::placeholders::_1);
      trace_ = c.TraceValue("boxed promise", changed);
   }

   ~BoxedPromise() {
      LOG(WARNING) << "killing BoxedPromise...";
   }

public:
   void Destroy() {
      trace_.Reset();
   }

   BoxedPromise* ChangedPromise(object cb) {
      changedCbs_.push_back(cb);
      return this;
   }

   void OnChange(const typename T::ValueType& value) {
      for (auto& cb : changedCbs_) {
         luabind::call_function<void>(cb, value);
      }
   }

private:
   dm::Guard               trace_;
   std::vector<object>     changedCbs_;
};

typedef SetPromise<om::BuildOrders::BuildOrderList> BuildOrdersBuildOrderListPromise;
typedef MapPromise<om::EntityContainer::Container> EntityContainerChildrenPromise;

BuildOrdersBuildOrderListPromise* TraceBuildOrdersInProgress(lua_State* L, om::BuildOrders& buildOrders)
{
   return new BuildOrdersBuildOrderListPromise(buildOrders.GetInProgress());
}

template <class T>
void BuildOrderReserveAdjacent(lua_State* L, T& s, math3d::ipoint3 location)
{
   math3d::ipoint3 block;

   bool reserved = s.ReserveAdjacent(location, block);
   object(L, reserved).push(L);
   object(L, block).push(L);
}

void EntityCarryBlockSetCarrying(om::CarryBlock& c, object o)
{
   if (luabind::type(o) == LUA_TNIL) {
      c.SetCarrying(om::EntityRef());
      return;
   }
   om::EntityRef e = object_cast<om::EntityRef>(o);
   auto entity = e.lock();
   if (entity) {
      c.SetCarrying(entity);
   }
}

bool EntityCarryBlockIsCarrying(om::CarryBlock& c)
{
   return c.GetCarrying().lock() != nullptr;
}

// xxx: cannot get luabind to automatically downgrade an om::WallRef to an om::BuildOrderRef
// when passed as a parameter to a function.  This is the crappy work around.
template <class T>
bool ConvertToBuildOrder(luabind::object o, om::BuildOrderRef& result)
{
   std::weak_ptr<T> obj;
   try {
      obj = object_cast<std::weak_ptr<T>>(o);
   } catch (cast_failed&) {
      return false;
   }
   result = static_pointer_cast<T>(obj.lock());
   return true;
}

template <typename T, dm::Guard (T::*M)(const char *, std::function<void()>) >
dm::Guard* TraceObject(T& obj, object o)
{
   // xxx - does this trace get nuked when ?  god, i hope so!
   static dm::Guard t = (obj.*M)("reason...", [=]() {
      call_function<void>(o);
   });
   return new dm::Guard();
}

template <class T>
bool IsValid(std::weak_ptr<T> e)
{
   return e.lock() != nullptr;
}

template <class T>
std::string WeakPtrToWatch(std::weak_ptr<T> o)
{
   std::ostringstream output;
   std::shared_ptr<T> obj = o.lock();
   if (obj) {
      output << *obj;
   } else {
      output << "Invalid " << T::GetObjectClassName() << " reference.";
   }
   return output.str();
}

template <class T>
std::string ToJsonUri(std::weak_ptr<T> o, luabind::object state)
{
   std::ostringstream output;
   std::shared_ptr<T> obj = o.lock();
   if (obj) {
      output << "\"/object/" << obj->GetObjectId() << "\"";
   } else {
      output << "null";
   }
   return output.str();
}

std::string BuildOrderToWatch(std::weak_ptr<om::BuildOrder> o)
{
   auto buildOrder = o.lock();
   std::ostringstream output;

   if (buildOrder) {
      output << "BuildOrder " << buildOrder;
   } else {
      output << "Invalid BuildOrder reference.";
   }
   return output.str();
}


template <class T>
bool EntityHasComponent(om::EntityRef e)
{
   auto entity = e.lock();
   return entity && entity->GetComponent<T>() != nullptr;
}

static luabind::object
EntityGetNativeComponent(lua_State* L, om::EntityPtr entity, std::string const& name)
{
#define OM_OBJECT(Clas, lower)  \
   if (name == #lower) { \
      auto component = entity->GetComponent<om::Clas>(); \
      if (!component) { \
         return luabind::object(); \
      } \
      return luabind::object(L, std::weak_ptr<om::Clas>(component)); \
   }
   OM_ALL_COMPONENTS
#undef OM_OBJECT
   return luabind::object();
}

std::string
GetLuaComponentUri(std::string name)
{
   std::string modname;
   size_t offset = name.find(':');

   if (offset != std::string::npos) {
      modname = name.substr(0, offset);
      name = name.substr(offset + 1, std::string::npos);

      JSONNode const& manifest = resources::ResourceManager2::GetInstance().LookupManifest(modname);
      return manifest["components"][name].as_string();
   }
   // xxx: throw an exception...
   return "";
}

static luabind::object
EntityGetLuaComponent(lua_State* L, om::EntityPtr entity, std::string const& name)
{
   GetLuaComponentUri(name);
   om::LuaComponentsPtr component = entity->GetComponent<om::LuaComponents>();
   if (component) {
      om::LuaComponentPtr lua_component = component->GetLuaComponent(name);
      if (lua_component) {
         return lua_component->GetLuaObject();
      }
   }
   return luabind::object();
}

luabind::object
EntityGetComponent(lua_State* L, om::EntityRef e, std::string name)
{
   luabind::object component;
   auto entity = e.lock();
   if (entity) {
      component = EntityGetNativeComponent(L, entity, name);
      if (!component.is_valid()) {
         component = EntityGetLuaComponent(L, entity, name);
      }
   }
   return component;
}

static luabind::object
EntityAddNativeComponent(lua_State* L, om::EntityPtr entity, std::string const& name)
{
#define OM_OBJECT(Clas, lower)  \
   if (name == #lower) { \
      auto component = entity->AddComponent<om::Clas>(); \
      return luabind::object(L, std::weak_ptr<om::Clas>(component)); \
   }
   OM_ALL_COMPONENTS
#undef OM_OBJECT
   return luabind::object();
}

static luabind::object
EntityAddLuaComponent(lua_State* L, om::EntityPtr entity, std::string const& name)
{
   using namespace luabind;

   object result;
   om::LuaComponentsPtr component = entity->AddComponent<om::LuaComponents>();
   om::LuaComponentPtr lua_component = component->GetLuaComponent(name);
   if (lua_component) {
      result = lua_component->GetLuaObject();
   } else {
      std::string uri = GetLuaComponentUri(name);
      object ctor = ScriptHost::GetInstance().LuaRequire(uri);
      if (ctor) {
         result = call_function<object>(ctor, om::EntityRef(entity));
         lua_component = component->AddLuaComponent(name);         
         lua_component->SetLuaObject(name, result);
      }
   }
   return result;
}

luabind::object
EntityAddComponent(lua_State* L, om::EntityRef e, std::string name)
{
   luabind::object component;
   auto entity = e.lock();
   if (entity) {
      component = EntityAddNativeComponent(L, entity, name);

      if (!component.is_valid()) {
         component = EntityAddLuaComponent(L, entity, name);
         ASSERT(component.is_valid());
      }
   }
   return component;
}


template <typename T>
class DmIterator
{
public:
   DmIterator(const T& container) : container_(container) {
      i_ = container_.begin();
   }

   void NextIteration(lua_State *L, luabind::object s, luabind::object var) {
      if (i_ != container_.end()) {
         luabind::object(L, *i_).push(L);
         i_++;
      } else {
         lua_pushnil(L);
      }
   }

private:
   const T& container_;
   typename T::ContainerType::const_iterator i_;
};

typedef DmIterator<dm::Queue<om::AutomationTaskPtr>> AutomationQueueIterator;

void AutomationQueueStartIteration(lua_State *L, om::AutomationQueue& queue)
{
   luabind::object(L, new AutomationQueueIterator(queue.GetContents())).push(L); // f
   luabind::object(L, 1).push(L); // s (ignored)
   luabind::object(L, 1).push(L); // var (ignored)
}

template <class T>
const char *GetClassNameFn(T& self)
{
   return T::GetObjectClassName();
}

void LuaObjectModel::RegisterType(lua_State* L)
{
#define ADD_OM_CLASS(Cls) \
      class_<om::Cls, std::weak_ptr<om::Cls> >(#Cls) \
         .def("is_valid",              &IsValid<om::Cls>) \
         .def("get_class_name",        &GetClassNameFn<om::Cls>) \
         .def("get_id",                &om::Cls::GetObjectId) \
         .def("__tojson",              &ToJsonUri<om::Cls>) \
         .def("__tostring",            &WeakPtrToWatch<om::Cls>) \
         .def("__towatch",             &WeakPtrToWatch<om::Cls>) \

#define ADD_OM_COMPONENT(Cls) \
         ADD_OM_CLASS(Cls) \
         .def("get_entity",            &om::Cls::GetEntityRef) \
         .def("extend",                &om::Cls::ExtendObject) \

// Things that do work...
// 1) class_<om::BuildOrder, std::weak_ptr<om::BuildOrder> >("BuildOrder")
//    Followed by no derived class implementations
//
// Things that don't work...
// 1) class_<om::BuildOrder, std::weak_ptr<om::BuildOrder> >("BuildOrder")
//    class_<om::Cls, om::BuildOrder, std::weak_ptr<om::Cls> >(#Cls)  (cannot find __tostring)
//
// 2) class_<om::BuildOrder, std::weak_ptr<om::BuildOrder> >("BuildOrder")
//    class_<om::Cls, std::weak_ptr<om::Cls> >(#Cls)  (cannot find __tostring or get_entity...)
//
// 3) ...
//    class_<om::Cls, std::weak_ptr<om::Cls> >(#Cls)  (cannot find __tostring or get_entity...)
//
// 4) ...
//    class_<om::Cls, om::BuildOrder, std::weak_ptr<om::Cls> >(#Cls)  (runtime failure... base class not registered)

#define ADD_OM_BUILD_ORDER(Cls) \
      ADD_OM_COMPONENT(Cls) \
         .def("get_material",             &om::Cls::GetMaterial) \
         .def("needs_more_work",          &om::Cls::NeedsMoreWork) \
         .def("should_tear_down",         &om::Cls::ShouldTearDown) \
         .def("get_construction_region",  &om::Cls::GetConstructionRegion) \
         .def("reserve_adjacent",         &BuildOrderReserveAdjacent<om::Cls>) \
         .def("construct_block",          &om::Cls::ConstructBlock) \
         .def("update_shape",             &om::Cls::UpdateShape) \
         .def("complete_to",              &om::Cls::CompleteTo) \


   module(L) [
      class_<dm::Guard>("Guard")
         ,
      class_<EntityContainerChildrenPromise>("EntityContainerChildrenPromise")
         .def("added",   &EntityContainerChildrenPromise::AddedPromise)
         .def("removed", &EntityContainerChildrenPromise::RemovedPromise)
         ,
      class_<BuildOrdersBuildOrderListPromise>("BuildOrdersBuildOrderListPromise")
         .def("added",   &BuildOrdersBuildOrderListPromise::AddedPromise)
         .def("removed", &BuildOrdersBuildOrderListPromise::RemovedPromise)
         ,
      ADD_OM_CLASS(Entity)
         .def("get_debug_name",  &om::Entity::GetDebugName)
         .def("set_debug_name",  &om::Entity::SetDebugName)
         .def("get_resource_uri",&om::Entity::GetResourceUri)
         .def("set_resource_uri",&om::Entity::SetResourceUri)
         .def("get_component" ,  &EntityGetComponent)
         .def("add_component" ,  &EntityAddComponent)
      ,
      ADD_OM_CLASS(Grid)
         .def("set_bounds",            &om::Grid::setBounds)
         .def("grow_bounds",           static_cast<void(om::Grid::*)(const math3d::ibounds3&)>(&om::Grid::GrowBounds))
         .def("set_block",             static_cast<void(om::Grid::*)(const math3d::ibounds3&, unsigned char)>(&om::Grid::setVoxel))
         .def("set_palette_entry",     &om::Grid::SetPaletteEntry)
         ,
      ADD_OM_CLASS(GridTile)
         ,
      ADD_OM_CLASS(Region)
         ,
      ADD_OM_COMPONENT(Clock)
         .def("set_time",              &om::Clock::SetTime)
         .def("get_time",              &om::Clock::GetTime)
         ,
      ADD_OM_CLASS(Effect)
         .def("get_name",              &om::Effect::GetName)
         .def("get_start_time",        &om::Effect::GetStartTime)
         .def("add_param",             &om::Effect::AddParam)
         .def("get_param",             &om::Effect::GetParam)
         ,
      ADD_OM_COMPONENT(EffectList)
         .def("add_effect",            &om::EffectList::AddEffect)
         .def("remove_effect",         &om::EffectList::RemoveEffect)
         ,
      ADD_OM_COMPONENT(BuildOrders)
         .def("get_projects",          &om::BuildOrders::GetProjects)
         .def("get_blueprints",        &om::BuildOrders::GetBlueprints)
         .def("get_in_progress",       &om::BuildOrders::GetInProgress)
         .def("add_blueprint",         &om::BuildOrders::AddBlueprint)
         .def("trace_in_progress",     TraceBuildOrdersInProgress)
         .def("start_project",         &om::BuildOrders::StartProject)
         ,
      ADD_OM_COMPONENT(RenderGrid)
         .def("set_grid",              &om::RenderGrid::SetGrid)
         ,
      ADD_OM_COMPONENT(GridCollisionShape)
         .def("set_grid",              &om::GridCollisionShape::SetGrid)
         ,
      ADD_OM_COMPONENT(Item)
         //.def("get_kind",              &om::Item::GetKind)
         //.def("set_kind",              &om::Item::SetKind)
         .def("set_stacks",            &om::Item::SetStacks)
         .def("get_stacks",            &om::Item::GetStacks)
         .def("set_max_stacks",        &om::Item::SetMaxStacks)
         .def("get_max_stacks",        &om::Item::GetMaxStacks)
         .def("get_material",          &om::Item::GetMaterial)
         .def("set_material",          &om::Item::SetMaterial)
         ,
      ADD_OM_COMPONENT(RenderRig)
         .def("add_rig",               &om::RenderRig::AddRig)
         .def("remove_rig",            &om::RenderRig::RemoveRig)
         .def("set_animation_table",   &om::RenderRig::SetAnimationTable)
         .def("get_animation_table",   &om::RenderRig::GetAnimationTable)
         .def("get_rigs",              &om::RenderRig::GetRigs)
         ,
      ADD_OM_COMPONENT(EntityContainer)
         .def("add_child",             &om::EntityContainer::AddChild)
         .def("remove_child",          &om::EntityContainer::RemoveChild)
         .def("get_children",          &om::EntityContainer::GetChildren)
         ,
      ADD_OM_COMPONENT(CarryBlock)
         .def("get_carrying",          &om::CarryBlock::GetCarrying)
         .def("is_carrying",           &EntityCarryBlockIsCarrying)
         .def("set_carrying",          &EntityCarryBlockSetCarrying)
         ,
      ADD_OM_COMPONENT(UnitInfo)
         .def("set_display_name",      &om::UnitInfo::SetDisplayName)
         .def("get_display_name",      &om::UnitInfo::GetDisplayName)
         .def("set_description",       &om::UnitInfo::SetDescription)
         .def("get_description",       &om::UnitInfo::GetDescription)
         .def("set_faction",           &om::UnitInfo::SetFaction)
         .def("get_faction",           &om::UnitInfo::GetFaction)
      ,
      ADD_OM_COMPONENT(Room)
         .def("set_bounds",            &om::Room::SetBounds)
      ,
      ADD_OM_COMPONENT(Portal)
         .def("set_portal",            &om::Portal::SetPortal)
      ,
      ADD_OM_BUILD_ORDER(Wall)
         .def("get_normal",               &om::Wall::GetNormal)
         .def("add_fixture",              &om::Wall::AddFixture)
      ,
      ADD_OM_BUILD_ORDER(Post)
      ,
      ADD_OM_BUILD_ORDER(Floor)
      ,
      ADD_OM_BUILD_ORDER(PeakedRoof)
      ,
      ADD_OM_BUILD_ORDER(Scaffolding)
      ,
      ADD_OM_BUILD_ORDER(Fixture)
         .def("get_kind",                 &om::Fixture::GetItemKind)
         .def("set_kind",                 &om::Fixture::SetItemKind)
         .def("get_item",                 &om::Fixture::GetItem)
         .def("set_item",                 &om::Fixture::SetItem)
      ,
      ADD_OM_COMPONENT(BuildOrderDependencies)
         .def("add_dependency",           &om::BuildOrderDependencies::AddDependency)
         .def("get_dependencies",         &om::BuildOrderDependencies::GetDependencies, return_stl_iterator)
      ,
      ADD_OM_COMPONENT(UserBehaviorQueue)
         .def("add_behavior",             &om::UserBehaviorQueue::AddUserBehavior)
      ,
      ADD_OM_COMPONENT(RenderInfo)
         .def("set_display_iconic",       &om::RenderInfo::SetDisplayIconic)
         .def("get_display_iconic",       &om::RenderInfo::GetDisplayIconic)
      ,
      ADD_OM_COMPONENT(Profession)
         .def("learn_recipe",             &om::Profession::LearnReceipe)
      ,
      ADD_OM_COMPONENT(AutomationTask)
         .def("get_handler",              &om::AutomationTask::GetHandler)
      ,
      ADD_OM_COMPONENT(AutomationQueue)
         .def("contents",                 &AutomationQueueStartIteration)
         .def("add_task",                 &om::AutomationQueue::AddTask)
         .def("clear",                    &om::AutomationQueue::Clear)
      ,
      class_<AutomationQueueIterator>("AutomationQueueIterator")
         .def("__call",    &AutomationQueueIterator::NextIteration)
      ,
      ADD_OM_COMPONENT(Paperdoll)
         .def("equip",                    &om::Paperdoll::SetSlot)
         .def("unequip",                  &om::Paperdoll::ClearSlot)
         .def("get_item_in_slot",         &om::Paperdoll::GetItemInSlot)
         .def("has_item_in_slot",         &om::Paperdoll::HasItemInSlot)
         .enum_("Slots")
         [
            value("MAIN_HAND",            om::Paperdoll::MAIN_HAND),
            value("WEAPON_SCABBARD",      om::Paperdoll::WEAPON_SCABBARD),
            value("NUM_SLOTS",            om::Paperdoll::NUM_SLOTS)
         ]
      ,
      ADD_OM_COMPONENT(Attributes)
         .def("get_attribute",            &om::Attributes::GetAttribute)
         .def("has_attribute",            &om::Attributes::HasAttribute)
         .def("set_attribute",            &om::Attributes::SetAttribute)
      ,
      ADD_OM_CLASS(Aura)
         .def("get_name",                 &om::Aura::GetName)
         .def("get_source",               &om::Aura::GetSource)
         .def("set_expire_time",          &om::Aura::SetExpireTime)
         .def("get_expire_time",          &om::Aura::GetExpireTime)
         .def("get_msg_handler",          &om::Aura::GetMsgHandler)
         .def("set_msg_handler",          &om::Aura::SetMsgHandler)
      ,
      ADD_OM_COMPONENT(AuraList)
         .def("create_aura",              &om::AuraList::CreateAura)
         .def("get_aura",                 &om::AuraList::GetAura)
      ,
      ADD_OM_CLASS(TargetTableEntry)
         .def("get_target",               &om::TargetTableEntry::GetTarget)
         .def("get_value",                &om::TargetTableEntry::GetValue)
         .def("get_expire_time",          &om::TargetTableEntry::GetExpireTime)
         .def("set_value",                &om::TargetTableEntry::SetValue)
         //.def("set_value",                &TargetTableEntrySetValue)
         .def("set_expire_time",          &om::TargetTableEntry::SetExpireTime)
      ,
      ADD_OM_CLASS(TargetTable)
         .def("get_entry",                &om::TargetTable::GetEntry)
         .def("add_entry",                &om::TargetTable::AddEntry)
         .def("set_name",                 &om::TargetTable::SetName)
      ,
      ADD_OM_CLASS(TargetTableGroup)
         .def("add_table",                &om::TargetTableGroup::AddTable)
      ,
      class_<om::TargetTableTop, std::shared_ptr<om::TargetTableTop> >("TargetTableTop")
         // xxx: add tostring!! and towatch!!!
         .def_readonly("target",      &om::TargetTableTop::target)
         .def_readonly("expires",     &om::TargetTableTop::expires)
         .def_readonly("value",       &om::TargetTableTop::value)
      ,
      ADD_OM_COMPONENT(TargetTables)
         .def("add_table",                &om::TargetTables::AddTable)
         .def("remove_table",             &om::TargetTables::RemoveTable)
         .def("get_top",                  &om::TargetTables::GetTop)

   ];

   // the new hotness...
   module(L) [
      om::Component::RegisterLuaType(L, "Component"),
      om::Inventory::RegisterLuaType(L, "Inventory"),
      om::WeaponInfo::RegisterLuaType(L, "WeaponInfo"),
      om::Sensor::RegisterLuaType(L, "Sensor"),
      om::SensorList::RegisterLuaType(L, "SensorList"),
      om::StockpileDesignation::RegisterLuaType(L, "StockpileDesignation"),
      om::Terrain::RegisterLuaType(L, "Terrain"),
      om::LuaComponents::RegisterLuaType(L, "LuaComponents"),

      dm::Set<om::EntityId>::RegisterLuaType(L, "Set<EntityId>"),
      dm::Set<std::string>::RegisterLuaType(L, "Set<String>"),
	  om::EntityContainer::Container::RegisterLuaType(L, "EntityChildrenContainerMap")
   ];
   om::Mob::RegisterLuaType(L),
   om::Destination::RegisterLuaType(L);
}
