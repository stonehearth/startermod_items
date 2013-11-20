#include "pch.h"
#include "item.ridl.h"
#include "mob.ridl.h"

using namespace ::radiant;
using namespace ::radiant::om;

void Item::ConstructObject()
{
   stacks_ = 1;
   max_stacks_ = 1;
}

void Item::ExtendObject(json::Node const& obj)
{
   JSONNode const& node = obj.GetNode();
   int count = obj.get<int>("stacks", *max_stacks_);
   stacks_ = count;
   max_stacks_ = count;
   category_ = obj.get<std::string>("category", *category_);
}
