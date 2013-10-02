#include "radiant.h"
#include <gtest/gtest.h>
#include "region.h"
#include "platform/utils.h"

using namespace ::radiant;
using namespace ::radiant::csg;

static const int PERF_TEST_DURATION_SECONDS = 2;

TEST(RegionTest, SubtractCube) {
   for (int x = 0; x < 3; x++) {
      for (int y = 0; y < 3; y++) {
         for (int z = 0; z < 3; z++) {
            Region3 r(Cube3::one.Scaled(3));
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

TEST(RegionPerfTest, RegionAddCubePerfTest) {
   srand(0);
   Region3 r;

   int count = 0;
   platform::timer t(PERF_TEST_DURATION_SECONDS * 1000);
   while (!t.expired()) {
      Point3 min, max;
      for (int i = 0; i < 3; i++) {
         min[i] = rand() % 1000;
         max[i] = rand() % 1000;
      }
      r += Cube3::Construct(min, max);
      count++;
   }
   std::cout << (count / (float)PERF_TEST_DURATION_SECONDS) << " cubes per second." << std::endl;
   RecordProperty("CubesPerSecond", count / PERF_TEST_DURATION_SECONDS);
}

TEST(RegionPerfTest, RegionAddUniqueCubePerfTest) {
   srand(0);
   Region3 r;

   int offset = 0, count = 0;
   platform::timer t(PERF_TEST_DURATION_SECONDS * 1000);
   while (!t.expired()) {
      Point3 min, max;
      for (int i = 0; i < 3; i++) {
         min[i] = offset;
         max[i] = offset + 1;
      }
      r.AddUnique(Cube3(min, max));
      count++;
      offset++;
   }
   std::cout << (count / (float)PERF_TEST_DURATION_SECONDS) << " cubes per second." << std::endl;
   RecordProperty("CubesPerSecond", count / PERF_TEST_DURATION_SECONDS);
}

TEST(RegionPerfTest, RegionSubCubePerfTest) {
   srand(0);
   Region3 r(csg::Cube3(csg::Point3(0, 0, 0), csg::Point3(1000, 1000, 1000)));

   int count = 0;
   platform::timer t(PERF_TEST_DURATION_SECONDS * 1000);
   while (!t.expired()) {
      Point3 min, max;
      for (int i = 0; i < 3; i++) {
         min[i] = rand() % 1000;
         max[i] = min[i] + rand() % 100;
      }
      r -= Cube3(min, max);
      count++;
   }
   std::cout << (count / (float)PERF_TEST_DURATION_SECONDS) << " cubes per second." << std::endl;
   RecordProperty("CubesPerSecond", count / PERF_TEST_DURATION_SECONDS);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
