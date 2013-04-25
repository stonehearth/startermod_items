#include "pch.h"
#include "lua_resource.h"
#include "resources/string_resource.h"
#include "resources/number_resource.h"
#include "resources/rig.h"
#include "resources/region2d.h"
#include "resources/animation.h"
#include "resources/data_resource.h"

using namespace ::radiant;
using namespace ::radiant::simulation;
using namespace ::luabind;


static luabind::object GetObjectResourceChild(lua_State* L, const resources::ObjectResource& obj, std::string key)
{
   return LuaResource::ConvertResourceToLua(L, obj.Get(key));
}

static luabind::object GetArrayResourceEntry(lua_State* L, const resources::ArrayResource& arr, int i)
{
   return LuaResource::ConvertResourceToLua(L, arr.Get(i));
}

void LuaArrayResourceStartIteration(lua_State *L, resources::ArrayResource& arr)
{
   luabind::object(L, new LuaArrayResourceIterator(arr)).push(L); // f
   luabind::object(L, 1).push(L); // s (ignored)
   luabind::object(L, 1).push(L); // var (ignored)
}

void LuaResource::RegisterType(lua_State* L)
{
   module(L) [
#if 0
      class_<LuaObjectResource>("ObjectResource")
         .def(tostring(self))
         .def_readonly("contents",  &LuaObjectResource::StartIteration)
         .def("get",                &LuaObjectResource::LookupItem)
      ,
      class_<LuaObjectResourceIterator>("ObjectResourceIterator")
         .def(tostring(self))
         .def("__call",    &LuaObjectResourceIterator::NextIteration)
      ,
      class_<LuaArrayResource>("ArrayResource")
         .def(tostring(self))
         .def_readonly("contents",  &LuaArrayResource::StartIteration)
         .def("get",                &LuaArrayResource::LookupItem)
      ,
#endif
      class_<resources::Resource>("Resource")
         ,
      class_<resources::ObjectResource, resources::Resource, std::shared_ptr<resources::Resource>>("ObjectResource")
         .def(tostring(self))
         .def("get", &GetObjectResourceChild)
      ,
      class_<resources::ArrayResource, resources::Resource, std::shared_ptr<resources::Resource>>("ArrayResource")
         .def(tostring(self))
         .def("get", &GetArrayResourceEntry)
         .def("contents",  &LuaArrayResourceStartIteration)
      ,
      class_<LuaArrayResourceIterator>("ArrayResourceIterator")
         .def("__call",    &LuaArrayResourceIterator::NextIteration)
      ,
      class_<resources::Rig, resources::Resource, std::shared_ptr<resources::Resource>>("RigResource")
         .def(tostring(self))
         .def("get_resource_identifier", &resources::Rig::GetResourceIdentifier)
         ,
      class_<resources::Region2d, resources::Resource, std::shared_ptr<resources::Resource>>("Region2dResource")
         .def(tostring(self))
      ,
      class_<resources::Animation, resources::Resource, std::shared_ptr<resources::Resource>>("AnimationResource")
         .def(tostring(self))
         .def("get_duration", &resources::Animation::GetDuration)
         ,
      class_<resources::DataResource, resources::Resource, std::shared_ptr<resources::Resource>>("DataResource")
         .def(tostring(self))
         .def("get_json", &resources::DataResource::GetJsonString)
   ];
}

luabind::object LuaResource::ConvertResourceToLua(lua_State *L, std::shared_ptr<resources::Resource> obj)
{
   luabind::object result;

   if (obj) {
      switch (obj->GetType()) {
      case resources::Resource::OBJECT:
         return luabind::object(L, static_cast<resources::ObjectResource*>(obj.get()));
         //return luabind::object(L, LuaObjectResource(static_cast<resources::ObjectResource*>(obj.get())));
      case resources::Resource::ARRAY:
         return luabind::object(L, static_cast<resources::ArrayResource*>(obj.get()));
         //return luabind::object(L, LuaArrayResource(static_cast<resources::ArrayResource*>(obj.get())));
      case resources::Resource::STRING:
         return luabind::object(L, static_cast<resources::StringResource*>(obj.get())->GetValue());
      case resources::Resource::NUMBER:
         return luabind::object(L, static_cast<resources::NumberResource*>(obj.get())->GetValue());
      case resources::Resource::RIG:
         return luabind::object(L, static_cast<resources::Rig*>(obj.get()));
      case resources::Resource::ANIMATION:
         return luabind::object(L, static_cast<resources::Animation*>(obj.get()));
      case resources::Resource::REGION_2D:
         return luabind::object(L, static_cast<resources::Region2d*>(obj.get()));
      case resources::Resource::EFFECT:
      case resources::Resource::RECIPE:
         return luabind::object(L, static_cast<resources::DataResource*>(obj.get()));
      default:
         ASSERT(false);
         break;
      }
   }

   return result;
}


/*
   The generic for statement works over functions, called iterators. On each iteration, the iterator function
   is called to produce a new value, stopping when this new value is nil. The generic for loop has the following syntax:

	   stat ::= for namelist in explist do block end
	   namelist ::= Name {‘,’ Name}

   A for statement like

        for var_1, ···, var_n in explist do block end

   is equivalent to the code:

        do
          local f, s, var = explist
          while true do
            local var_1, ···, var_n = f(s, var)
            if var_1 == nil then break end
            var = var_1
            block
          end
        end
*/

LuaObjectResource::LuaObjectResource(const resources::ObjectResource* obj) :
   obj_(obj)
{
}


LuaObjectResourceIterator::LuaObjectResourceIterator(const resources::ObjectResource* obj) :
   obj_(obj)
{
   iter_ = obj_->GetContents().begin();
}

void LuaObjectResourceIterator::NextIteration(lua_State *L, luabind::object s, luabind::object var)
{
   if (iter_ != obj_->GetContents().end()) {
      luabind::object(L, iter_->first).push(L);
      LuaResource::ConvertResourceToLua(L, iter_->second).push(L);
      iter_++;
   } else {
      lua_pushnil(L);
      lua_pushnil(L);
   }
}

void LuaObjectResource::StartIteration(lua_State *L)
{
   luabind::object(L, new LuaObjectResourceIterator(obj_)).push(L); // f
   luabind::object(L, 1).push(L); // s (ignored)
   luabind::object(L, 1).push(L); // var (ignored)
}

luabind::object LuaObjectResource::LookupItem(lua_State *L, luabind::object arg0, luabind::object deflt)
{
   std::string name = object_cast<std::string>(arg0);
   auto resource = obj_->Get(name);
   if (resource) {
      return LuaResource::ConvertResourceToLua(L, resource);
   }
   return deflt;
}

LuaArrayResourceIterator::LuaArrayResourceIterator(const resources::ArrayResource& obj) :
   obj_(obj),
   i_(0)
{
}

void LuaArrayResourceIterator::NextIteration(lua_State *L, luabind::object s, luabind::object var)
{
   if (i_ < obj_.GetSize()) {
      LuaResource::ConvertResourceToLua(L, obj_[i_]).push(L);
      i_++;
   } else {
      lua_pushnil(L);
   }
}
