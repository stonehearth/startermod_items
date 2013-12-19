#include "radiant.h"
#include "om/region.h"
#include "csg/point.h"
#include "nav_grid.h"
#include "dm/map_trace.h"
#include "om/components/terrain.ridl.h"
#include "terrain_tracker.h"

using namespace radiant;
using namespace radiant::phys;

TerrainTracker::TerrainTracker(NavGrid& ng, om::EntityPtr entity, om::TerrainPtr terrain) :
   CollisionTracker(ng, entity),
   terrain_(terrain)
{
}

void TerrainTracker::Initialize()
{
   om::TerrainPtr terrain = terrain_.lock();
   if (terrain) {
      trace_ = terrain->TraceZones("nav grid", GetNavGrid().GetTraceCategory())
            ->OnAdded([this](om::Terrain::ZoneKey const& offset, om::Terrain::ZoneValue const& region) {
               GetNavGrid().AddTerrainTileTracker(GetEntity(), offset, region);
            })
            ->OnRemoved([this](om::Terrain::ZoneKey const& removed) {
               NOT_YET_IMPLEMENTED();
            })
            ->PushObjectState();
   }
}

void TerrainTracker::MarkChanged()
{
   // nothing to do...
}

csg::Region3 TerrainTracker::GetOverlappingRegion(csg::Cube3 const& bounds) const
{
   return csg::Region3::empty;
}
