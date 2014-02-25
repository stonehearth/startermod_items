#include "pch.h"
#include "protocols/store.pb.h"
#include "controller_object.h"

using namespace radiant;
using namespace radiant::lua;

ControllerObject::ControllerObject()
{
}

ControllerObject::ControllerObject(std::string const& source, luabind::object object) :
   lua_source_(source),
   lua_object_(object)
{
}

std::string const& ControllerObject::GetLuaSource() const
{
   return lua_source_;
}

luabind::object ControllerObject::GetLuaObject() const
{
   return lua_object_;
}

void ControllerObject::SetLuaObject(luabind::object obj) const {
   lua_object_ = obj;
}

void ControllerObject::SaveValue(Protocol::LuaControllerObject *msg) const
{
   msg->set_lua_script(lua_source_);
}

void ControllerObject::LoadValue(const Protocol::LuaControllerObject &msg)
{
   lua_source_ = msg.lua_script();
}

std::ostream& lua::operator<<(std::ostream& os, ControllerObject const& o)
{
   return (os << "[lua controller object]");
}
