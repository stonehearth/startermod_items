#ifndef _RADIANT_CLIENT_RENDER_TERRAIN_TILE_H
#define _RADIANT_CLIENT_RENDER_TERRAIN_TILE_H

#include <memory>
#include "namespace.h"
#include "om/region.h"
#include "csg/region_tools.h"
#include "dm/dm.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class RenderTerrainTile
{
public:
   typedef std::unordered_map<int, csg::Region2> Geometry;

public:
   RenderTerrainTile(RenderTerrain& terrain, csg::Point3 const& location, om::Region3BoxedRef region);
   ~RenderTerrainTile();

   int UpdateClipPlanes(int clip_height);
   void UpdateGeometry(int clip_height);

   void SetClipPlane(csg::RegionTools3::Plane direction, csg::Region2 const* clipPlane);
   csg::Region2 const* GetClipPlane(csg::RegionTools3::Plane direction);
   Geometry const& GetGeometry(csg::RegionTools3::Plane direction);

   void AddCut(dm::ObjectId cutId, csg::Region3 const* cut);
   void RemoveCut(dm::ObjectId cutId);

private:
   NO_COPY_CONSTRUCTOR(RenderTerrainTile);
   csg::Region2 const* GetClipPlaneFor(csg::PlaneInfo3 const& pi);
   csg::Region3 const& ComputeCutTerrainRegion(csg::Region3& storage, int clip_height, csg::Region2& cross_section) const;

private:
   RenderTerrain&          _terrain;
   csg::Point3             _location;
   om::Region3BoxedRef     _region;
   csg::Region3            _renderRegion;
   csg::Region2            _clipPlanes[csg::RegionTools3::NUM_PLANES];
   csg::Region2 const*     _neighborClipPlanes[csg::RegionTools3::NUM_PLANES];
   Geometry                _geometry[csg::RegionTools3::NUM_PLANES];
   dm::TracePtr            _trace;
   csg::RegionTools3       _regionTools;
   std::unordered_map<dm::ObjectId, csg::Region3 const*> _cutMap;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_TERRAIN_TILE_H
