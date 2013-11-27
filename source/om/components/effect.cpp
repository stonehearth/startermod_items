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

void Effect::Init(std::string name, int start)
{
   name_ = name;
   start_time_ = start;
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
