#ifndef _RADIANT_LUA_REGISTER_H
#define _RADIANT_LUA_REGISTER_H

#include "namespace.h"
#include "om/object_formatter/object_formatter.h" // xxx: for GetPathToObject...

BEGIN_RADIANT_LUA_NAMESPACE

#define IMPLEMENT_TRIVIAL_TOSTRING(Cls)                           \
std::ostream& operator<<(std::ostream& os, Cls const& f)          \
{                                                                 \
   return os << "[" << ::radiant::lua::GetTypeName<Cls>() << "]"; \
}

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
std::string TypePtr_ToJson(std::shared_ptr<T> obj, luabind::object state)
{
   std::ostringstream output;
   if (obj) {
      output << '"' << om::ObjectFormatter().GetPathToObject(obj) << '"';
   } else {
      output << "null";
   }
   return output.str();
}

template <class T>
std::string TypeRef_ToJson(std::weak_ptr<T> o, luabind::object state)
{
   std::shared_ptr<T> obj = o.lock();
   if (obj) {
      return TypePtr_ToJson(obj, state);
   }
   std::ostringstream output;
   output << "null";
   return output.str();
}

template <typename T>
luabind::class_<T> RegisterType(const char* name = nullptr)
{
   name = name ? name : GetTypeName<T>();
   auto type = luabind::class_<T>(name);
   type
      .def(tostring(luabind::self))
      .def("get_type_name",  &Type_GetTypeName<T>);
   return type;
}

template <typename T>
luabind::class_<T, std::shared_ptr<T>> RegisterTypePtr(const char* name = nullptr)
{
   name = name ? name : GetTypeName<T>();
   return luabind::class_<T, std::shared_ptr<T>>(name)
      .def(tostring(luabind::self))
      .def("get_type_name",  &Type_GetTypeName<T>);
}

template <typename T>
luabind::class_<T, std::weak_ptr<T>> RegisterObject(const char* name = nullptr)
{
   name = name ? name : GetTypeName<T>();
   return luabind::class_<T, std::weak_ptr<T>>(name)
      .def(tostring(luabind::self))
      .def("__tojson",       &TypeRef_ToJson<T>)
      .def("get_id",         &T::GetObjectId)
      .def("get_type_name",  &Type_GetTypeName<T>);
}

template <typename T>
luabind::class_<T, std::shared_ptr<T>> RegisterObjectPtr(const char* name = nullptr)
{
   name = name ? name : GetTypeName<T>();
   return luabind::class_<T, std::shared_ptr<T>>(name)
      .def(tostring(luabind::self))
      .def("__tojson",       &TypePtr_ToJson<T>)
      .def("get_id",         &T::GetObjectId)
      .def("get_type_name",  &Type_GetTypeName<T>);
}

template<class Derived, class Base>
luabind::class_<Derived, Base, std::weak_ptr<Derived>> RegisterDerivedObject(const char* name = nullptr)
{
   name = name ? name : GetTypeName<Derived>();
   return luabind::class_<Derived, Base, std::weak_ptr<Derived>>(name)
      .def(tostring(luabind::self))
      .def("__tojson",       &TypePtr_ToJson<Derived>)
      .def("get_id",         &Derived::GetObjectId)
      .def("get_type_name",  &Type_GetTypeName<Derived>);
}

END_RADIANT_LUA_NAMESPACE

#endif //  _RADIANT_LUA_REGISTER_H