#ifndef _RADIANT_LUA_REGISTER_H
#define _RADIANT_LUA_REGISTER_H

#include "lib/lua/lua.h"
#include "lib/json/node.h"
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
   output << '"' << om::ObjectFormatter().GetPathToObject(obj) << '"';
   return output.str();
}

template <class T>
std::string WeakGameObjectToJson(std::weak_ptr<T> o, luabind::object state)
{
   return StrongGameObjectToJson(o.lock(), state);
}

// Used for putting value based C++ types into Lua (e.g. csg::Point3, csg::Cube3, etc.)
template <typename T>
luabind::class_<T> RegisterType(const char* name = nullptr)
{
   name = name ? name : GetShortTypeName<T>();
   return luabind::class_<T>(name)
      .def(tostring(luabind::self))
      .def("__tojson",       &TypeToJson<T>)
      .def("get_type_name",  &GetTypeName<T>);
}

template <typename T>
luabind::class_<T, std::shared_ptr<T>> RegisterTypePtr(const char* name = nullptr)
{
   name = name ? name : GetShortTypeName<T>();
   return luabind::class_<T, std::shared_ptr<T>>(name)
      .def(tostring(luabind::self))
      .def("__tojson",       &TypePointerToJson<T>)
      .def("get_type_name",  &GetTypeName<T>);
}

template <typename T>
luabind::class_<T, std::shared_ptr<T>> RegisterStrongGameObject(const char* name = nullptr)
{
   name = name ? name : GetShortTypeName<T>();
   return luabind::class_<T, std::shared_ptr<T>>(name)
      .def(tostring(luabind::self))
      .def("__tojson",       &StrongGameObjectToJson<T>)
      .def("get_id",         &T::GetObjectId)
      .def("get_type_name",  &GetTypeName<T>);
}

template <typename T>
luabind::class_<T, std::weak_ptr<T>> RegisterWeakGameObject(const char* name = nullptr)
{
   name = name ? name : GetShortTypeName<T>();
   return luabind::class_<T, std::weak_ptr<T>>(name)
      .def(tostring(luabind::self))
      .def("__tojson",       &WeakGameObjectToJson<T>)
      .def("get_id",         &T::GetObjectId)
      .def("get_type_name",  &GetTypeName<T>);
}


// xxx: see if we can overload this with RegisterWeakGameObject
template<class Derived, class Base>
luabind::class_<Derived, Base, std::weak_ptr<Derived>> RegisterWeakGameObjectDerived(const char* name = nullptr)
{
   name = name ? name : GetShortTypeName<Derived>();
   return luabind::class_<Derived, Base, std::weak_ptr<Derived>>(name)
      .def(tostring(luabind::self))
      .def("__tojson",       &WeakGameObjectToJson<Derived>)
      .def("get_id",         &Derived::GetObjectId)
      .def("get_type_name",  &GetTypeName<Derived>);
}

END_RADIANT_LUA_NAMESPACE

#endif //  _RADIANT_LUA_REGISTER_H