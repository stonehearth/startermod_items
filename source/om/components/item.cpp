#include "radiant.h"
#include "item.ridl.h"
#include "mob.ridl.h"

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& operator<<(std::ostream& os, Item const& o)
{
   return (os << "[Item]");
}

void Item::ConstructObject()
{
   Component::ConstructObject();
   stacks_ = 1;
   max_stacks_ = 1;
}

void Item::LoadFromJson(json::Node const& obj)
{
   int count = obj.get<int>("stacks", *max_stacks_);
   stacks_ = count;
   max_stacks_ = count;
}

void Item::SerializeToJson(json::Node& node) const
{
   Component::SerializeToJson(node);

   node.set("stacks", GetStacks());
}
