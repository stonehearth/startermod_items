#include "radiant.h"
//#include "radiant_test.h"
#include <gtest/gtest.h>
#include "region.h"

//RADIANT_DEFINE_TEST(csg)

using namespace ::radiant::csg;

TEST(RegionTest, SubtractCube) {
   for (int x = 0; x < 3; x++) {
      for (int y = 0; y < 3; y++) {
         for (int z = 0; z < 3; z++) {
            Region3 r(Cube3::one.Scale(3));
            Point3 pt(x, y, z);
            r -= pt;
			EXPECT_EQ(26, r.GetArea()) << "area of a 3x3 cube after removing " << pt << " should be 8";
            for (int tx = 0; tx < 3; tx++) {
               for (int ty = 0; ty < 3; ty++) {
                  for (int tz = 0; tz < 3; tz++) {
                     Point3 tester(tx, ty, tz);
                     std::ostringstream test;
                     EXPECT_EQ(pt != tester, r.Intersects(tester)) << "checking " << tester << " after removing point " << pt << " from region";
                  }
               }
            }
         }
      }
   }
}
