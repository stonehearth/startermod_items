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

void Clock::ExtendObject(json::Node const& obj)
{
}

