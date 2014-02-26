#ifndef _RADIANT_LUA_REGISTER_H
#define _RADIANT_LUA_REGISTER_H

#include "lib/lua/lua.h"
#include "lib/json/node.h"
#include "lib/lua/script_host.h"
#include "om/object_formatter/object_formatter.h" // xxx: for GetPathToObject...

BEGIN_RADIANT_LUA_NAMESPACE

#define IMPLEMENT_TRIVIAL_TOSTRING(Cls)                                 \
std::ostream& operator<<(std::ostream& os, Cls const& f)                \
{                                                                       \
   return os << "[" << ::radiant::GetShortTypeName<Cls>() << "]";  \
}

template <class T>
const char* GetTypeName(T const&)
{
   return typeid(T).name();
}

template <class T>
const char* GetClassTypeName()
{
   return typeid(T).name();
}

template <class T>
const size_t GetTypeId(T const&)
{
   return typeid(T).hash_code();
}

template <class T>
const size_t GetClassTypeId()
{
   return typeid(T).hash_code();
}

template <class T>
std::string TypeToJson(T const& obj, luabind::object state)
{
   return json::encode(obj).write();
}

template <class T>
std::string TypePointerToJson(std::shared_ptr<T> obj, luabind::object state)
{
   if (!obj) {
      return "null";
   }
   return json::encode(obj).write();
}

template <class T>
std::string StrongGameObjectToJson(std::shared_ptr<T> obj, luabind::object state)
{
   if (!obj) {
      return "null";
   }
   std::ostringstream output;
   output << '"' << ::radiant::om::ObjectFormatter().GetPathToObject(obj) << '"';
   return output.str();
}

template <class T>
bool WeakPtr_IsValid(std::weak_ptr<T> o)
{
   return !o.expired();
}

template<typename T>
static dm::ObjectId SharedGetObjectId(std::shared_ptr<T> o)
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
   return SharedGetObjectId(o.lock());
}

template<typename T>
static bool operator==(std::weak_ptr<T> lhs, std::weak_ptr<T> rhs)
{
   return lhs.lock() == rhs.lock();
}

template <class T>
std::string WeakGameObjectToJson(std::weak_ptr<T> o, luabind::object state)
{
   return StrongGameObjectToJson(o.lock(), state);
}


template <class T>
std::string TypeRepr(T const& obj)
{
   return lua::Repr<T>(obj);
}

template <class T>
std::string TypePointerRepr(std::shared_ptr<T> const& obj)
{
   // These things can't be saved.  Tough!
   return "nil";
}

template <class T>
std::string StrongGameObjectRepr(std::shared_ptr<T> const& obj)
{
   if (obj) {
      return BUILD_STRING("radiant.get_object('" << obj->GetStoreAddress() << "')");
   }
   return "nil";
}

template <class T>
std::string WeakGameObjectRepr(std::weak_ptr<T> const& obj)
{
   return StrongGameObjectRepr(obj.lock());
}

// Used for putting value based C++ types into Lua (e.g. csg::Point3, csg::Cube3, etc.)
template <typename T>
luabind::class_<T> RegisterType(const char* name = nullptr)
{
   name = name ? name : GetShortTypeName<T>();
   return luabind::class_<T>(name)
      .def(tostring(luabind::self))
      .def("__tojson",       &TypeToJson<T>)
      .def("__repr",         &TypeRepr<T>)
      .def("get_type_id",    &GetTypeId<T>)
      .def("get_type_name",  &GetTypeName<T>)
      .scope [
         def("get_type_id",   &GetClassTypeId<T>),
         def("get_type_name", &GetClassTypeName<T>) 
      ];
}

template <typename T>
luabind::class_<T, std::shared_ptr<T>> RegisterTypePtr(const char* name = nullptr)
{
   name = name ? name : GetShortTypeName<T>();
   return luabind::class_<T, std::shared_ptr<T>>(name)
      .def(tostring(luabind::self))
      .def("__tojson",       &TypePointerToJson<T>)
      .def("__repr",         &TypePointerRepr<T>)
      .def("get_type_id",    &GetTypeId<T>)
      .def("get_type_name",  &GetTypeName<T>)
      .scope [
         def("get_type_id",   &GetClassTypeId<T>),
         def("get_type_name", &GetClassTypeName<T>) 
      ];
}

template <typename T>
luabind::class_<T, std::shared_ptr<T>> RegisterStrongGameObject(lua_State* L, const char* name = nullptr)
{
   lua::ScriptHost* host = lua::ScriptHost::GetScriptHost(L);
   host->AddObjectToLuaConvertor(T::GetObjectTypeStatic(), [](lua_State* L, dm::ObjectPtr obj) {
      std::shared_ptr<T> typed_obj = std::static_pointer_cast<T>(obj);
      return luabind::object(L, typed_obj);
   });

   name = name ? name : GetShortTypeName<T>();
   return luabind::class_<T, std::shared_ptr<T>>(name)
      .def(tostring(luabind::self))
      .def("__tojson",       &StrongGameObjectToJson<T>)
      .def("__repr",         &StrongGameObjectRepr<T>)
      .def("get_id",         &SharedGetObjectId<T>)
      .def("get_type_id",    &GetTypeId<T>)
      .def("get_type_name",  &GetTypeName<T>)
      .scope [
         def("get_type_id",   &GetClassTypeId<T>),
         def("get_type_name", &GetClassTypeName<T>) 
      ];
}

template <typename T>
luabind::class_<T, std::weak_ptr<T>> RegisterWeakGameObject(lua_State* L, const char* name = nullptr)
{
   lua::ScriptHost* host = lua::ScriptHost::GetScriptHost(L);
   host->AddObjectToLuaConvertor(T::GetObjectTypeStatic(), [](lua_State* L, dm::ObjectPtr obj) {
      std::shared_ptr<T> typed_obj = std::static_pointer_cast<T>(obj);
      return luabind::object(L, std::weak_ptr<T>(typed_obj));
   });

   name = name ? name : GetShortTypeName<T>();
   return luabind::class_<T, std::weak_ptr<T>>(name)
      .def(tostring(luabind::self))
      .def("is_valid",       &WeakPtr_IsValid<T>)
      .def("__tojson",       &WeakGameObjectToJson<T>)
      .def("__repr",         &WeakGameObjectRepr<T>)
      .def("get_id",         &WeakGetObjectId<T>)
      .def("get_type_id",    &GetTypeId<T>)
      .def("get_type_name",  &GetTypeName<T>)
      .def("equals",         (bool (*)(std::weak_ptr<T>, std::weak_ptr<T>))&operator==)
      .scope [
         def("get_type_id",   &GetClassTypeId<T>),
         def("get_type_name", &GetClassTypeName<T>) 
      ];
}


// xxx: see if we can overload this with RegisterWeakGameObject
template<class Derived, class Base>
luabind::class_<Derived, Base, std::weak_ptr<Derived>> RegisterWeakGameObjectDerived(lua_State* L, const char* name = nullptr)
{
   lua::ScriptHost* host = lua::ScriptHost::GetScriptHost(L);
   host->AddObjectToLuaConvertor(Derived::GetObjectTypeStatic(), [](lua_State* L, dm::ObjectPtr obj) {
      std::shared_ptr<Derived> typed_obj = std::static_pointer_cast<Derived>(obj);
      return luabind::object(L, std::weak_ptr<Derived>(typed_obj));
   });

   name = name ? name : GetShortTypeName<Derived>();
   return luabind::class_<Derived, Base, std::weak_ptr<Derived>>(name)
      .def(tostring(luabind::self))
      .def("is_valid",       &WeakPtr_IsValid<Derived>)
      .def("__tojson",       &WeakGameObjectToJson<Derived>)
      .def("__repr",         &WeakGameObjectRepr<Derived>)
      .def("get_id",         &WeakGetObjectId<Derived>)
      .def("get_type_id",    &GetTypeId<Derived>)
      .def("get_type_name",  &GetTypeName<Derived>)
      .scope [
         def("get_type_id",   &GetClassTypeId<Derived>),
         def("get_type_name", &GetClassTypeName<Derived>) 
      ];
}

END_RADIANT_LUA_NAMESPACE

#endif //  _RADIANT_LUA_REGISTER_H