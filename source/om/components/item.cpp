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
   AddRecordField("material", material_);
}

void Item::Construct(json::ConstJsonObject const& obj)
{
   JSONNode const& node = obj.GetNode();
   int count = node["stacks"].as_int();
   SetStacks(count);
   SetMaxStacks(count);
   SetMaterial(node["material"].as_string());
}
