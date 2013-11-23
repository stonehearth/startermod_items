#include "pch.h"
#include "clock.ridl.h"

using namespace ::radiant;
using namespace ::radiant::om;

void Clock::ConstructObject()
{
   time_ = 0;
}

void Clock::ExtendObject(json::Node const& obj)
{
}

