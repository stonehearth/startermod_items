#ifndef _RADIANT_LUA_REGISTER_H
#define _RADIANT_LUA_REGISTER_H

#include "lib/lua/lua.h"
#include "lib/json/node.h"
#include "lib/lua/script_host.h"
#include "lib/typeinfo/type.h"

BEGIN_RADIANT_LUA_NAMESPACE

#define IMPLEMENT_TRIVIAL_TOSTRING(Cls)                                 \
std::ostream& operator<<(std::ostream& os, Cls const& f)                \
{                                                                       \
   return os << "[" << ::radiant::GetShortTypeName<Cls>() << "]";  \
}

template <class T>
size_t GetTypeHashCode(T const&)
{
   return typeid(T).hash_code();
}

template <class T>
const size_t GetClassTypeId()
{
   return typeid(T).hash_code();
}

template <class T>
std::string GetTypeName(T const&)
{
   return std::string(typeid(T).name());
}

template <class T>
std::string GetStaticTypeName()
{
   return std::string(typeid(T).name());
}

template <class T>
std::string TypeToJson(T const& obj)
{
   return json::encode(obj).write();
}

template <class T>
std::string TypePointerToJson(std::shared_ptr<T> obj)
{
   if (!obj) {
      return "null";
   }
   // this is somewhat stupid... we conver the objec to json, then write
   // it, then pass it back to someone who might actually have wanted a
   // json object (and will decode it!).  it would be nice to just return
   // a JSONNode, but what if the caller is from lua?  Ug.
   return json::encode(obj).write();
}

template <class T>
std::string StrongGameObjectToJson(std::shared_ptr<T> obj)
{
   if (!obj) {
      return "null";
   }
   return obj->GetStoreAddress();
}

template <class T>
bool WeakPtr_IsValid(std::weak_ptr<T> o)
{
   return !o.expired();
}

template<typename T>
static dm::ObjectId StrongGetObjectId(std::shared_ptr<T> o)
{
   if (o) {
      return o->GetObjectId();
   }
   throw std::invalid_argument("invalid reference in native get_id");
   return 0;
}

template<typename T>
static dm::ObjectId WeakGetObjectId(std::weak_ptr<T> o)
{
   return StrongGetObjectId(o.lock());
}

template<typename T>
static std::string StrongGetObjectAddress(std::shared_ptr<T> o)
{
   if (o) {
      return o->GetStoreAddress();
   }
   throw std::invalid_argument("invalid reference in native get_address");
   return "";
}

template<typename T>
static std::string WeakGetObjectAddress(std::weak_ptr<T> o)
{
   return StrongGetObjectAddress(o.lock());
}

template<typename T>
static luabind::object StrongSerializeToJson(lua_State* L, std::shared_ptr<T> obj)
{
   luabind::object result;
   if (obj) {
      lua::ScriptHost* scriptHost = lua::ScriptHost::GetScriptHost(L);
      json::Node json_data;
      obj->SerializeToJson(json_data);
      result = scriptHost->JsonToLua(json_data);
   }
   return result;
}

template<typename T>
static luabind::object WeakSerializeToJson(lua_State* L, std::weak_ptr<T> o)
{
   return StrongSerializeToJson(L, o.lock());
}

template<typename T>
static bool operator==(std::weak_ptr<T> lhs, std::weak_ptr<T> rhs)
{
   return lhs.lock() == rhs.lock();
}

template <class T>
std::string WeakGameObjectToJson(std::weak_ptr<T> o)
{
   return StrongGameObjectToJson(o.lock());
}

template <class T>
static std::shared_ptr<lua::TraceWrapper>
StrongTraceGameObject(std::shared_ptr<T> o, const char* reason, dm::TraceCategories c)
{
   if (o)  {
      auto trace = o->TraceObjectChanges(reason, c);
      return std::make_shared<lua::TraceWrapper>(trace);
   }
   throw std::invalid_argument("invalid reference in native :trace_changes");
}

template <class T>
static std::shared_ptr<lua::TraceWrapper>
StrongTraceGameObjectAsync(std::shared_ptr<T> o, const char* reason)
{
   return StrongTraceGameObject(o, reason, dm::LUA_ASYNC_TRACES);
}

template <class T>
static std::shared_ptr<lua::TraceWrapper>
WeakTraceGameObject(std::weak_ptr<T> o, const char* reason, dm::TraceCategories c)
{
   return StrongTraceGameObject(o.lock(), reason, c);
}

template <class T>
static std::shared_ptr<lua::TraceWrapper>
WeakTraceGameObjectAsync(std::weak_ptr<T> o, const char* reason)
{
   return StrongTraceGameObject(o.lock(), reason, dm::LUA_ASYNC_TRACES);
}

// Used for putting value based C++ types into Lua (e.g. csg::Point3, csg::Cube3, etc.)
template <typename T>
luabind::class_<T> RegisterType_NoTypeInfo(const char* name)
{
   return luabind::class_<T>(name)
      .def(tostring(luabind::const_self))
      .def("get_type_id",    &GetTypeHashCode<T>)
      .def("get_type_name",  &GetTypeName<T>)      
      .scope [
         def("get_type_id",   &GetClassTypeId<T>),
         def("get_type_name", &GetStaticTypeName<T>) 
      ];
}

// Used for putting value based C++ types into Lua (e.g. csg::Point3, csg::Cube3, etc.)
template <typename T>
luabind::class_<T> RegisterType(const char* name)
{
   typedef typeinfo::Type<T> Type;

   return luabind::class_<T>(name)
      .def(tostring(luabind::const_self))
      .def("__get_userdata_type_id", &Type::GetTypeId)
      .def("__tojson",       &TypeToJson<T>)
      .def("get_type_id",    &GetTypeHashCode<T>)
      .def("get_type_name",  &GetTypeName<T>)      
      .scope [
         def("get_type_id",   &GetClassTypeId<T>),
         def("get_type_name", &GetStaticTypeName<T>)
      ];
}

template <typename T>
luabind::class_<T, std::shared_ptr<T>> RegisterTypePtr_NoTypeInfo(const char* name)
{
   return luabind::class_<T, std::shared_ptr<T>>(name)
      .def(tostring(luabind::const_self))
      .def("get_type_id",    &GetTypeHashCode<T>)
      .def("get_type_name",  &GetTypeName<T>)
      .scope [
         def("get_type_id",   &GetClassTypeId<T>),
         def("get_type_name", &GetStaticTypeName<T>) 
      ];
}

template <typename T>
luabind::class_<T, std::shared_ptr<T>> RegisterTypePtr(const char* name)
{
   typedef typeinfo::Type<std::shared_ptr<T>> Type;
   return luabind::class_<T, std::shared_ptr<T>>(name)
      .def(tostring(luabind::const_self))
      .def("__get_userdata_type_id", &Type::GetTypeId)
      .def("__tojson",       &TypePointerToJson<T>)
      .def("get_type_id",    &GetTypeHashCode<T>)
      .def("get_type_name",  &GetTypeName<T>)
      .scope [
         def("get_type_id",   &GetClassTypeId<T>),
         def("get_type_name", &GetStaticTypeName<T>)
      ];
}

// registers both the shared_ptr and weak_ptr versions of the class methods
#define REGISTER_DUAL_CLASS_METHODS(name, method_suffix) \
   .def(name, &Strong ## method_suffix) \
   .def(name, &Weak ## method_suffix)

template <typename T>
luabind::class_<T, std::shared_ptr<T>> RegisterStrongGameObject(lua_State* L, const char* name)
{
   typedef typeinfo::Type<std::shared_ptr<T>> Type;
   ASSERT(name);

   lua::ScriptHost* host = lua::ScriptHost::GetScriptHost(L);
   host->AddObjectToLuaConvertor(T::GetObjectTypeStatic(), [](lua_State* L, dm::ObjectPtr obj) {
      std::shared_ptr<T> typed_obj = std::static_pointer_cast<T>(obj);
      return luabind::object(L, typed_obj);
   });

   return luabind::class_<T, std::shared_ptr<T>>(name)
      .def(tostring(luabind::const_self)) 
      .def(luabind::self == luabind::self)
      .def("__get_userdata_type_id", &Type::GetTypeId)
      REGISTER_DUAL_CLASS_METHODS("__tojson", GameObjectToJson<T>)
      REGISTER_DUAL_CLASS_METHODS("get_id", GetObjectId<T>)
      REGISTER_DUAL_CLASS_METHODS("get_address", GetObjectAddress<T>)
      REGISTER_DUAL_CLASS_METHODS("serialize", SerializeToJson<T>)
      REGISTER_DUAL_CLASS_METHODS("trace", TraceGameObject<T>)
      REGISTER_DUAL_CLASS_METHODS("trace", TraceGameObjectAsync<T>)
      .def("get_type_id",    &GetTypeHashCode<T>)
      .def("get_type_name",  &GetTypeName<T>)
      .scope [
         def("get_type_id",   &GetClassTypeId<T>),
         def("get_type_name", &GetStaticTypeName<T>)
      ];
}

template <typename T>
luabind::class_<T, std::weak_ptr<T>> RegisterWeakGameObject(lua_State* L, const char* name)
{
   typedef typeinfo::Type<std::weak_ptr<T>> Type;
   ASSERT(name);

   lua::ScriptHost* host = lua::ScriptHost::GetScriptHost(L);
   host->AddObjectToLuaConvertor(T::GetObjectTypeStatic(), [](lua_State* L, dm::ObjectPtr obj) {
      std::shared_ptr<T> typed_obj = std::static_pointer_cast<T>(obj);
      return luabind::object(L, std::weak_ptr<T>(typed_obj));
   });
   return luabind::class_<T, std::weak_ptr<T>>(name)
      .def(tostring(luabind::const_self))
      .def(luabind::self == luabind::self)
      .def("is_valid",       &WeakPtr_IsValid<T>)
      .def("__get_userdata_type_id", &Type::GetTypeId)
      .def("__tojson",       &WeakGameObjectToJson<T>)
      .def("get_id",         &WeakGetObjectId<T>)
      .def("get_address",    &WeakGetObjectAddress<T>)
      .def("get_type_id",    &GetTypeHashCode<T>)
      .def("get_type_name",  &GetTypeName<T>)
      .def("serialize",      &WeakSerializeToJson<T>)
      .def("trace",          &WeakTraceGameObject<T>)
      .def("trace",          &WeakTraceGameObjectAsync<T>)
      .def("equals",         (bool (*)(std::weak_ptr<T>, std::weak_ptr<T>))&operator==)
      .scope [
         def("get_type_id",   &GetClassTypeId<T>),
         def("get_type_name", &GetStaticTypeName<T>)
      ];
}


// xxx: see if we can overload this with RegisterWeakGameObject
template<class Derived, class Base>
luabind::class_<Derived, Base, std::weak_ptr<Derived>> RegisterWeakGameObjectDerived(lua_State* L, const char* name)
{
   typedef typeinfo::Type<std::weak_ptr<Derived>> Type;
   ASSERT(name);

   lua::ScriptHost* host = lua::ScriptHost::GetScriptHost(L);
   host->AddObjectToLuaConvertor(Derived::GetObjectTypeStatic(), [](lua_State* L, dm::ObjectPtr obj) {
      std::shared_ptr<Derived> typed_obj = std::static_pointer_cast<Derived>(obj);
      return luabind::object(L, std::weak_ptr<Derived>(typed_obj));
   });
   return luabind::class_<Derived, Base, std::weak_ptr<Derived>>(name)
      .def(tostring(luabind::const_self))
      .def(luabind::self == luabind::self)
      .def("is_valid",       &WeakPtr_IsValid<Derived>)
      .def("__get_userdata_type_id", &Type::GetTypeId)
      .def("__tojson",       &WeakGameObjectToJson<Derived>)
      .def("get_id",         &WeakGetObjectId<Derived>)
      .def("get_address",    &WeakGetObjectAddress<Derived>)
      .def("get_type_id",    &GetTypeHashCode<Derived>)
      .def("get_type_name",  &GetTypeName<Derived>)
      .def("serialize",      &WeakSerializeToJson<Derived>)
      .def("trace",          &WeakTraceGameObject<Derived>)
      .def("trace",          &WeakTraceGameObjectAsync<Derived>)
      .scope [
         def("get_type_id",   &GetClassTypeId<Derived>),
         def("get_type_name", &GetStaticTypeName<Derived>)
      ];
}

static luabind::object GetPointerToCFunction(lua_State *L, lua_CFunction f)
{
   luabind::detail::stack_pop pop(L, 1);
   lua_pushcfunction(L, f);
   return luabind::object(luabind::from_stack(L, -1));
};

END_RADIANT_LUA_NAMESPACE

#endif //  _RADIANT_LUA_REGISTER_H