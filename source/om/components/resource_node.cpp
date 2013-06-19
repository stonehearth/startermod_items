#include "pch.h"
#include "resource_node.h"
#include "mob.h"

using namespace ::radiant;
using namespace ::radiant::om;

void ResourceNode::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("resource", resource_);
   AddRecordField("durability", durability_);
}

std::vector<math3d::ipoint3> ResourceNode::GetHarvestStandingLocations() const
{
   std::vector<math3d::ipoint3> result;

   static const int r = 3;
   math3d::ipoint3 origin(0, 0, 0);
   om::MobPtr mob = GetEntity().GetComponent<om::Mob>();
   if (mob) {
      origin = math3d::ipoint3(mob->GetLocation());
   }

   // xxx: converting this to a region of 4 rectangles would be much more
   // efficient in the long run (i think...)
   for (int i = -r; i <= r; i++) {
      result.push_back(math3d::ipoint3(origin.x + i, origin.y, origin.z + r));
      result.push_back(math3d::ipoint3(origin.x + i, origin.y, origin.z - r));
      result.push_back(math3d::ipoint3(origin.x + r, origin.y, origin.z + i));
      result.push_back(math3d::ipoint3(origin.x - r, origin.y, origin.z + i));
   }
   return result;
}

std::string ResourceNode::GetResource() const
{
   return resource_;
}

void ResourceNode::SetResource(std::string res)
{
   resource_ = res;
}

int ResourceNode::GetDurability() const
{
   return durability_;
}

void ResourceNode::SetDurability(int durability)
{
   durability_ = durability;
}
