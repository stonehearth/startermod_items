#include "pch.h"
#include "portal.h"

using namespace ::radiant;
using namespace ::radiant::om;

void Portal::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("region", region_);
}

void Portal::SetPortal(csg::Region2 const& region)
{
   region_ = region;
}

void Portal::Construct(json::ConstJsonObject const& obj)
{
   csg::Region2 rgn;
   JSONNode const& node = obj.GetNode();

   for (const JSONNode& rc : node["rects"]) {
      rgn += csg::Rect2(csg::Point2(rc[0][0].as_int(), rc[0][1].as_int()),
                        csg::Point2(rc[1][0].as_int() + 1, rc[1][1].as_int() + 1));
   }
   SetPortal(rgn);
}
