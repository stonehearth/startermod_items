#include "pch.h"
#include "clock.h"

using namespace ::radiant;
using namespace ::radiant::om;

Clock::Clock()
{
}

Clock::~Clock()
{
}

void Clock::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("now", now_);
   now_ = 0;
}
