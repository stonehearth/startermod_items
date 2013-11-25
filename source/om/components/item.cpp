#include "pch.h"
#include "item.h"
#include "mob.h"

using namespace ::radiant;
using namespace ::radiant::om;

void Item::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("kind", kind_);
   AddRecordField("stacks", stacks_);
   AddRecordField("maxStacks", maxStacks_);
   AddRecordField("category", category_);

   stacks_ = 1;
   maxStacks_ = 1;
}

void Item::ExtendObject(json::Node const& obj)
{
   int count = obj.get<int>("stacks", *maxStacks_);
   stacks_ = count;
   maxStacks_ = count;
   category_ = obj.get<std::string>("category", *category_);
}
