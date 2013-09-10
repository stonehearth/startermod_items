#ifndef _RADIANT_LUA_REGISTER_H
#define _RADIANT_LUA_REGISTER_H

#include "namespace.h"
#include "om/object_formatter/object_formatter.h" // xxx: for GetPathToObject...

BEGIN_RADIANT_LUA_NAMESPACE

template <class Derived>
const char* GetTypeName()
{
   const char* name = typeid(Derived).name();
   const char* last = strrchr(name, ':');
   return last ? last + 1 : name;
}

template <class T>
const char* Type_GetTypeName(T const&)
{
   return typeid(T).name();
}

template <class T>
std::string Type_ToJson(std::weak_ptr<T> o, luabind::object state)
{
   std::ostringstream output;
   std::shared_ptr<T> obj = o.lock();
   if (obj) {
      output << '"' << om::ObjectFormatter().GetPathToObject(obj) << '"';
   } else {
      output << "null";
   }
   return output.str();
}

template <class T>
std::string Type_ToWatch(std::weak_ptr<T> o)
{
   std::ostringstream output;
   std::shared_ptr<T> obj = o.lock();
   if (obj) {
      output << obj.get();
   } else {
      output << "Invalid " << GetTypeName<T>() << " reference.";
   }
   return output.str();
}

template <typename LuaBindType, typename T>
void RegisterBaseMethods(LuaBindType& type)
{
   type
      .def(tostring(self))
      .def("__towatch",      &Type_ToWatch<T>)
      .def("__tojson",       &Type_ToJson<T>)
      .def("get_id",         &T::GetObjectId)
      .def("get_type_name",  &Type_GetTypeName<T>);
}

template <typename T>
luabind::class_<T> RegisterType(const char* name = nullptr)
{
   name = name ? name : GetTypeName<T>();
   auto type = luabind::class_<T>(name);
   type
      .def("__towatch",      &Type_ToWatch<T>)
      .def("get_type_name",  &Type_GetTypeName<T>);
   return type;
}

template <typename T>
luabind::class_<T, std::shared_ptr<T>> RegisterTypePtr(const char* name = nullptr)
{
   name = name ? name : GetTypeName<T>();
   auto type = luabind::class_<T, std::shared_ptr<T>>(name);
   type
      .def("__towatch",      &Type_ToWatch<T>)
      .def("get_type_name",  &Type_GetTypeName<T>);
   return type;
}

template <typename T>
luabind::class_<T, std::weak_ptr<T>> RegisterObject(const char* name = nullptr)
{
   name = name ? name : GetTypeName<T>();
   auto type = luabind::class_<T, std::weak_ptr<T>>(name);
   lua::RegisterBaseMethods<decltype(type), T>(type);
   return type;
}

template<class Derived, class Base>
luabind::class_<Derived, Base, std::weak_ptr<Derived>> RegisterDerivedObject(const char* name = nullptr)
{
   name = name ? name : GetTypeName<Derived>();
   return class_<Derived, Base, std::weak_ptr<Derived>>(name);
}

void RegisterBasicTypes(lua_State* L); // xxx: Must GO!

END_RADIANT_LUA_NAMESPACE

#endif //  _RADIANT_LUA_REGISTER_H