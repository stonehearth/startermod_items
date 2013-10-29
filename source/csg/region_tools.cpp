#include "pch.h"
#include "color.h"
#include "region_tools.h"

using namespace ::radiant;
using namespace ::radiant::csg;

std::vector<PlaneInfo<int, 2>> RegionToolsTraitsBase<int, 2, RegionToolsTraits<int, 2>>::planes;
std::vector<PlaneInfo<int, 3>> RegionToolsTraitsBase<int, 3, RegionToolsTraits<int, 3>>::planes;
std::vector<PlaneInfo<float, 2>> RegionToolsTraitsBase<float, 2, RegionToolsTraits<float, 2>>::planes;
std::vector<PlaneInfo<float, 3>> RegionToolsTraitsBase<float, 3, RegionToolsTraits<float, 3>>::planes;

// works around the fact that visual studio does not yet support
// C++11 container type initializers
struct RegionToolsInitializer
{
   RegionToolsInitializer() {
      Initialize2();
      Initialize3();
   }

   void Initialize2() {
      struct { int coord, x; } planes[] = {
         { 0, 1 },
         { 1, 0 }
      };
      for (const auto& item : planes) {
         RegionToolsTraits<int, 2>::PlaneInfo plane;
         plane.x = item.x;
         plane.reduced_coord = item.coord;
         RegionToolsTraits2::planes.push_back(plane);
         RegionToolsTraits2f::planes.push_back(ToFloat(plane));
      }
   }

   void Initialize3() {
      struct { int coord, x, y; } planes[] = {
         { 0, 2, 1 },
         { 1, 0, 2 },
         { 2, 0, 1 },
      };
      for (const auto& item : planes) {
         RegionToolsTraits<int, 3>::PlaneInfo plane;
         plane.x = item.x;
         plane.y = item.y;
         plane.reduced_coord = item.coord;
         RegionToolsTraits3::planes.push_back(plane);
         RegionToolsTraits3f::planes.push_back(ToFloat(plane));
      }
   }
};

RegionToolsInitializer ri;
