#include "pch.h"
#include "clock.ridl.h"

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& operator<<(std::ostream& os, Clock const& o)
{
   return (os << "[Clock " << o.GetTime() << "]");
}

void Clock::ConstructObject()
{
   Component::ConstructObject();
   time_ = 0;
}

void Clock::LoadFromJson(json::Node const& obj)
{
}

void Clock::SerializeToJson(json::Node& node) const
{
   Component::SerializeToJson(node);
}
