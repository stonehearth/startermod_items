// csg.cpp : Defines the entry point for the console application.
//

#include <windows.h>

#define ASSERT(cond)   \
   do { \
      if (!(cond)) { \
         DebugBreak(); \
      } \
   } while (0)

#include <sstream>
#include <functional>
#include "stdafx.h"
#include "../region.h"

using namespace ::radiant::csg;

#define VERIFY(cond, test)   \
   do { \
      bool success = (cond); \
      const char* result = success ? "OK" : "FAILED"; \
      printf("%8s : %s.\n", result, std::string(test).c_str()); \
      if (!success) { DebugBreak(); } \
   } while (0)

std::function<void()> tests[] = {
   []() {
      Cube3 cube(Point3(0, 0, 0), Point3(1, 2, 3));
      VERIFY(cube.GetArea() == 6, "cube area");
      VERIFY(cube.Scale(3).GetArea() == 162, "cube scale");
   },
   []() {
      Region3 r;
      r.Add(Cube3::one);
      VERIFY(r.GetArea() == 1, "region area");
   },
   []() {
      for (int x = 0; x < 3; x++) {
         for (int y = 0; y < 3; y++) {
            for (int z = 0; z < 3; z++) {
               Region3 r(Cube3::one.Scale(3));
               Point3 pt(x, y, z);
               r -= pt;
               for (int tx = 0; tx < 3; tx++) {
                  for (int ty = 0; ty < 3; ty++) {
                     for (int tz = 0; tz < 3; tz++) {
                        Point3 tester(tx, ty, tz);
                        std::ostringstream test;
                        test << "checking " << tester << " after removing point " << pt << " from region";
                        VERIFY(r.Intersects(Point3(tx, ty, tz)) == (pt != tester), test.str());
                     }
                  }
               }
            }
         }
      }
   },
};

int _tmain(int argc, _TCHAR* argv[])
{
   Region3 r;

   for (auto test : tests) {
      test();
   }
	return 0;
}

