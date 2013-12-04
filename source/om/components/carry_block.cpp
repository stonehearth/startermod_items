#include "pch.h"
#include "carry_block.ridl.h"

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& operator<<(std::ostream& os, CarryBlock const& o)
{
   return (os << "[CarryBlock]");
}


void CarryBlock::ExtendObject(json::Node const& obj)
{
}

bool CarryBlock::IsCarrying() const
{
   return GetCarrying().lock() != nullptr;
}

void CarryBlock::ClearCarrying()
{
   carrying_ = EntityRef();
}
