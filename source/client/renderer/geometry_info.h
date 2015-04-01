#ifndef _RADIANT_CLIENT_GEOMETRY_INFO_H
#define _RADIANT_CLIENT_GEOMETRY_INFO_H

#include "namespace.h"
#include "h3d_resource_types.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class GeometryInfo {
public:
   enum {
      MAX_LOD_LEVELS = 2     // @klochek looked into > 2 and found it just wasn't worth it.
   };

   enum GeometryType {
      NO_GEOMETRY,
      HORDE_GEOMETRY,
      VOXEL_GEOMETRY,
   };

   int vertexIndices[MAX_LOD_LEVELS + 1];
   int indexIndicies[MAX_LOD_LEVELS + 1];
   int levelCount;
   bool noInstancing;
   SharedGeometry geo;
   GeometryType type;

   GeometryInfo() : levelCount(0), geo(0), type(NO_GEOMETRY), noInstancing(false) {
      memset(vertexIndices, 0, sizeof vertexIndices);
      memset(indexIndicies, 0, sizeof indexIndicies);
   }
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_GEOMETRY_INFO_H
