#include "radiant.h"
#include <functional>
#include <unordered_set>
#include <gtest/gtest.h>
#include "region.h"
#include "iterators.h"
#include "platform/utils.h"
#include "lib/perfmon/perfmon.h"
#include "lib/perfmon/frame.h"
#include "lib/perfmon/counter.h"
#include "lib/perfmon/timer.h"
#include "csg/region_tools.h"

using namespace ::radiant;
using namespace ::radiant::csg;

static const int PERF_TEST_DURATION_SECONDS = 2;

typedef std::unordered_set<csg::Point3, csg::Point3::Hash> Point3Set;

template <typename T>
void CheckIteratorPoints(T const& r, std::function<Point3Set()> pointGenerator)
{
   auto points = pointGenerator();
   auto range = csg::EachPoint(r);
   auto begin = range.begin(), end = range.end();

   while (begin != end) {
      auto i = points.find(*begin);
      EXPECT_NE(i, points.end()) << "could not find point " << *begin << " in expected points";
      points.erase(*begin);
      ++begin;
   }
   EXPECT_EQ(points.empty(), true) << "iterator stopped early";
}

template <typename T>
void AddCubePointsToSet(T const& c, Point3Set& s)
{
   auto cube = csg::ToInt(c);
   for (int x = cube.min.x; x < cube.max.x; x++) { 
      for (int y = cube.min.y; y < cube.max.y; y++) {
         for (int z = cube.min.z; z < cube.max.z; z++) {
            s.insert(csg::Point3(x, y, z));
         }
      }
   }
}

TEST(CubeIterator, Empty) {
   auto generator = []() -> Point3Set {
      return Point3Set();
   };
   CheckIteratorPoints(csg::Cube3::zero, generator);
   CheckIteratorPoints(csg::Cube3f::zero, generator);
}

TEST(CubeIterator, ZeroArea) {
   csg::Cube3 empty(csg::Point3::zero, csg::Point3(0, 10, 0), 0);
   auto generator = []() -> Point3Set {
      return Point3Set();
   };
   CheckIteratorPoints(empty, generator);
   CheckIteratorPoints(csg::ToFloat(empty), generator);
}

TEST(CubeIterator, Trivial) {
   csg::Cube3 cube(csg::Point3::zero, csg::Point3::one.Scaled(4), 0);
   auto generator = [&]() -> Point3Set {
      Point3Set points;
      AddCubePointsToSet(cube, points);
      return points;
   };
   CheckIteratorPoints(cube, generator);
   CheckIteratorPoints(csg::ToFloat(cube), generator);
}

TEST(CubeIterator, FloatingPoint) {
   csg::Cube3f cube(csg::Point3f::one.Scaled(-1.5), csg::Point3f::one.Scaled(1.5), 0);
   auto generator = [&]() -> Point3Set {
      Point3Set points;
      AddCubePointsToSet(csg::ToInt(cube), points);
      return points;
   };

   CheckIteratorPoints(csg::ToInt(cube), generator);
   CheckIteratorPoints(cube, generator);
}

TEST(RegionIterator, Empty) {
   auto generator = []() -> Point3Set {
      return Point3Set();
   };
   CheckIteratorPoints(csg::Region3::zero, generator);
   CheckIteratorPoints(csg::Region3f::zero, generator);
}

TEST(RegionIterator, EmptyCube) {
   auto generator = [&]() -> Point3Set {
      return Point3Set();
   };
   csg::Region3 empty;
   csg::Region3f emptyf;
   const_cast<csg::Region3::CubeVector&>(empty.GetContents()).push_back(csg::Cube3::zero);
   const_cast<csg::Region3f::CubeVector&>(emptyf.GetContents()).push_back(csg::Cube3f::zero);
   CheckIteratorPoints(empty, generator);
   CheckIteratorPoints(emptyf, generator);
}

TEST(RegionIterator, EmptyCubeEmbeddedStart) {
   csg::Cube3 cubea(csg::Point3::zero, csg::Point3::one.Scaled(2), 0);
   csg::Cube3 cubeb(csg::Point3::one.Scaled(2), csg::Point3::one.Scaled(4), 0);
   auto generator = [&]() -> Point3Set {
      Point3Set points;
      AddCubePointsToSet(cubea, points);
      AddCubePointsToSet(cubeb, points);
      return points;
   };

  
   // Start...
   csg::Region3 r;
   const_cast<csg::Region3::CubeVector&>(r.GetContents()).push_back(csg::Cube3::zero);
   r.AddUnique(cubea);
   r.AddUnique(cubeb);
   CheckIteratorPoints(r, generator);
   CheckIteratorPoints(csg::ToFloat(r), generator);
}

TEST(RegionIterator, EmptyCubeEmbeddedMiddle) {
   csg::Cube3 cubea(csg::Point3::zero, csg::Point3::one.Scaled(2), 0);
   csg::Cube3 cubeb(csg::Point3::one.Scaled(2), csg::Point3::one.Scaled(4), 0);
   auto generator = [&]() -> Point3Set {
      Point3Set points;
      AddCubePointsToSet(cubea, points);
      AddCubePointsToSet(cubeb, points);
      return points;
   };

   // Middle...
   csg::Region3 r;
   r.AddUnique(cubea);
   const_cast<csg::Region3::CubeVector&>(r.GetContents()).push_back(csg::Cube3::zero);
   r.AddUnique(cubeb);
   CheckIteratorPoints(r, generator);
   CheckIteratorPoints(csg::ToFloat(r), generator);
}

TEST(RegionIterator, EmptyCubeEmbeddedEnd) {
   csg::Cube3 cubea(csg::Point3::zero, csg::Point3::one.Scaled(2), 0);
   csg::Cube3 cubeb(csg::Point3::one.Scaled(2), csg::Point3::one.Scaled(4), 0);
   auto generator = [&]() -> Point3Set {
      Point3Set points;
      AddCubePointsToSet(cubea, points);
      AddCubePointsToSet(cubeb, points);
      return points;
   };

   // End...
   csg::Region3 r;
   r.AddUnique(cubea);
   r.AddUnique(cubeb);
   const_cast<csg::Region3::CubeVector&>(r.GetContents()).push_back(csg::Cube3::zero);
   CheckIteratorPoints(r, generator);
   CheckIteratorPoints(csg::ToFloat(r), generator);
}

TEST(RegionIterator, Trivial) {
   csg::Cube3 cube(csg::Point3::zero, csg::Point3::one.Scaled(4), 0);
   auto generator = [&]() -> Point3Set {
      Point3Set points;
      AddCubePointsToSet(cube, points);
      return points;
   };

   CheckIteratorPoints(csg::Region3(cube), generator);
   CheckIteratorPoints(csg::Region3f(csg::ToFloat(cube)), generator);
}

TEST(RegionIterator, FloatingPoint) {
   csg::Cube3f cube(csg::Point3f::one.Scaled(-1.5), csg::Point3f::one.Scaled(1.5), 0);
   auto generator = [&]() -> Point3Set {
      Point3Set points;
      AddCubePointsToSet(csg::ToInt(cube), points);
      return points;
   };

   CheckIteratorPoints(csg::Region3(csg::ToInt(cube)), generator);
   CheckIteratorPoints(csg::Region3f(cube), generator);
}

TEST(RegionIterator, Multiple) {
   csg::Cube3 cubea(csg::Point3::zero, csg::Point3::one.Scaled(2), 0);
   csg::Cube3 cubeb(csg::Point3::one.Scaled(2), csg::Point3::one.Scaled(4), 0);
   auto generator = [&]() -> Point3Set {
      Point3Set points;
      AddCubePointsToSet(cubea, points);
      AddCubePointsToSet(cubeb, points);
      return points;
   };

   csg::Region3 r;
   r.AddUnique(cubea);
   r.AddUnique(cubeb);
   CheckIteratorPoints(r, generator);
   CheckIteratorPoints(csg::ToFloat(r), generator);
}

TEST(RegionIterator, MultipleFloatingPoint) {
   csg::Cube3f cubea(csg::Point3f::zero, csg::Point3f::one.Scaled(1.5), 0);
   csg::Cube3f cubeb(csg::Point3f(0.1, 1.5, 0.1), csg::Point3f(1.4, 3, 1.4));
   auto generator = [&]() -> Point3Set {
      Point3Set points;
      AddCubePointsToSet(cubea, points);
      AddCubePointsToSet(cubeb, points);
      return points;
   };

   csg::Region3f r;
   r.Add(cubea);
   r.Add(cubeb);
   //CheckIteratorPoints(csg::ToInt(r), generator);
   CheckIteratorPoints(r, generator);
}

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
   Region3 r(Cube3(Point3(0, 0, 0), Point3(1000, 1000, 1000)));

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

struct ExpectedTime { const char* name; uint expected; };
static const int perfmon_gap_time = 400;

void RunPerfmonTest(ExpectedTime *times, std::function<void()> const& execute)
{
   auto verify_times = [=](perfmon::Frame* f) {
      static const uint threshold = 10;
      for (ExpectedTime* t = times; t->name != nullptr; t++) {
         perfmon::Counter* c = f->GetCounter(t->name);
         uint value = perfmon::CounterToMilliseconds(c->GetValue());
         uint diff = std::abs((int)t->expected - (int)value);
         EXPECT_LE(diff, threshold) << "counter " << t->name << " value " << value << " not close enough to " << t->expected;
      }
   };
    
   core::Guard g = perfmon::OnFrameEnd(verify_times);
   {
      perfmon::BeginFrame(true);
      execute();
      perfmon::BeginFrame(true);
   }
}

static void WaitFor(int ms)
{
   perfmon::Timer t;
   auto get_elapsed = [&t]() {
      return perfmon::CounterToMilliseconds(t.GetElapsed());
   };

   t.Start();
   int sleep_time = std::max(ms - 50, 0);
   if (sleep_time > 0) {
      Sleep(sleep_time);   // sleep for most of the interval...
   }
   while (get_elapsed() < (uint)ms) {
      continue;            // spin for the rest
   }
}

static void Wait()
{
   WaitFor(perfmon_gap_time);
}


TEST(Perfmon, Timeline) {
   struct ExpectedTime times[] = {
      { "frame", perfmon_gap_time },
      { 0, 0 }
   };
   RunPerfmonTest(times, []() {
      perfmon::TimelineCounterGuard tg("frame");
      Wait();
   });
}

TEST(Perfmon, TimelineVector) {
   struct ExpectedTime times[] = {
      { "top_half", perfmon_gap_time },
      { "bot_half", perfmon_gap_time },
      { 0, 0 }
   };
   RunPerfmonTest(times, []() {
      perfmon::TimelineCounterGuard tg("top_half");
      Wait();
      perfmon::SwitchToCounter("bot_half");
      Wait();
   });
}

TEST(Perfmon, TimelineNestedInner) {
   struct ExpectedTime times[] = {
      { "top_half", perfmon_gap_time },
      { "middle_1",   perfmon_gap_time },
      { "middle_2",   perfmon_gap_time },
      { "bot_half", perfmon_gap_time },
      { 0, 0 }
   };
   RunPerfmonTest(times, []() {
      perfmon::TimelineCounterGuard tg("top_half");
      Wait();
      {
         perfmon::TimelineCounterGuard tg("middle_1");
         Wait();
         perfmon::SwitchToCounter("middle_2");
         Wait();
      }
      perfmon::SwitchToCounter("bot_half");
      Wait();
   });
}

TEST(RegionTools2D, ForEachPlane) {
   Region2 r2;
   r2.AddUnique(Rect2(Point2(1, 2), Point2(3, 4)));

   RegionTools2 tools;
   tools.ForEachPlane(r2, [=](Region1 const& r1, RegionTools2::PlaneInfo const& pi) {
      for (const auto &c : csg::EachCube(r1)) {
         LOG(test, 0) << c << " " << pi.reduced_value;
      }
   });
}

TEST(RegionTools2, ForEachEdge) {
   Region2 r2;
   r2.AddUnique(Rect2(Point2(1, 2), Point2(3, 4)));

   RegionTools2 tools;
   tools.ForEachEdge(r2, [=](csg::EdgeInfo2 const& edge_info) {
      LOG(test, 0) << edge_info.min << " " << edge_info.max << " " << edge_info.normal;
   });
}

TEST(RegionTools3, ForEachPlane) {
   Region3 r3;
   r3.AddUnique(Cube3(Point3(0, 1, 2), Point3(100, 101, 102)));

   RegionTools3 tools;
   tools.ForEachPlane(r3, [=](Region2 const& r2, RegionTools3::PlaneInfo const& pi) {
      for (const auto &c : csg::EachCube(r2)) {
         LOG(test, 0) << c << " " << pi.reduced_value;
      }
   });
}

TEST(RegionTools3, ForEachEdge) {
   Region3 r3;
   r3.AddUnique(Cube3(Point3(0, 1, 2), Point3(100, 101, 102)));

   RegionTools3 tools;
   int i = 0;
   tools.ForEachEdge(r3, [&](csg::EdgeInfo3 const& edge_info) {
      LOG(test, 0) << std::setw(3) << i++ << " " << edge_info.min << " " << edge_info.max << " " << edge_info.normal;
   });
}


void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
   return std::malloc(size);
}

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
   return std::malloc(size);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
