#include "pch.h"
#include "effect.ridl.h"
#include "om/entity.h"
#include "om/selection.h"

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& operator<<(std::ostream& os, const Effect& o)
{
   os << "[Effect " << o.GetObjectId() << " name:" << o.GetName() << "]";
   return os;
}

void Effect::Init(int effect_id, std::string name, int start)
{
   name_ = name;
   start_time_ = start;
   effect_id_ = effect_id;
}

void Effect::LoadFromJson(json::Node const& node)
{
}

void Effect::SerializeToJson(json::Node& node) const
{
   Record::SerializeToJson(node);
   node.set("name", GetName());
}

void Effect::AddParam(std::string name, luabind::object o)
{
   Selection s;
   s.FromLuaObject(o);
   params_.Add(name, s);
}

const Selection& Effect::GetParam(std::string param) const
{
   static const Selection null;
   auto i = params_.find(param);
   return i != params_.end() ? i->second : null;
}
