#include "pch.h"
#include "color.h"
#include "region_tools.h"

using namespace ::radiant;
using namespace ::radiant::csg;

std::vector<PlaneInfo<int, 2>> RegionToolsTraitsBase<int, 2, RegionToolsTraits<int, 2>>::planes;
std::vector<PlaneInfo<int, 3>> RegionToolsTraitsBase<int, 3, RegionToolsTraits<int, 3>>::planes;

// works around the fact that visual studio does not yet support
// C++11 container type initializers
struct RegionToolsInitializer
{
   RegionToolsInitializer() {
      Initialize2();
      Initialize3();
   }

   void Initialize2() {
      RegionToolsTraits<int, 2>::PlaneInfo r2pi[] = {
         { 0xd3adb33f, 0xd3adb33f, 0, 1  },
         { 0xd3adb33f, 0xd3adb33f, 1, 0  },
      };
      for (const auto& item : r2pi) {
         RegionToolsTraitsBase<int, 2, RegionToolsTraits<int, 2>>::planes.push_back(item);
      }
   }

   void Initialize3() {
      RegionToolsTraits<int, 3>::PlaneInfo r3pi[] = {
         { 0xd3adb33f, 0xd3adb33f, 0, 2, 1 },
         { 0xd3adb33f, 0xd3adb33f, 1, 0, 2 },
         { 0xd3adb33f, 0xd3adb33f, 2, 0, 1 },
      };
      for (const auto& item : r3pi) {
         RegionToolsTraitsBase<int, 3, RegionToolsTraits<int, 3>>::planes.push_back(item);
      }
   }
};

RegionToolsInitializer ri;
