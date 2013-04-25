#include "pch.h"
#include "build_order.h"
#include "build_order_dependencies.h"

using namespace ::radiant;
using namespace ::radiant::om;

void BuildOrderDependencies::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("dependencies",           dependencies_);
}

void BuildOrderDependencies::AddDependency(EntityRef e)
{
   dependencies_.Insert(e);
}

const BuildOrderDependencies::Dependencies& BuildOrderDependencies::GetDependencies() const
{
   return dependencies_;
}
