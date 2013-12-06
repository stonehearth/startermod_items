#include "radiant.h"
#include <functional>
#include <gtest/gtest.h>
#include "region.h"
#include "platform/utils.h"
#include "lib/perfmon/perfmon.h"
#include "lib/perfmon/frame.h"
#include "lib/perfmon/counter.h"
#include "lib/perfmon/timer.h"
#include "csg/region_tools.h"

using namespace ::radiant;
using namespace ::radiant::csg;

#if 0

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

void RunPerfmonTest(ExpectedTime *times, std::function<void()> execute)
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
      perfmon::BeginFrame();
      execute();
      perfmon::BeginFrame();
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
   while (get_elapsed() < ms) {
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

#endif

#if 0
TEST(RegionTools2D, ForEachPlane) {
   Region2 r2;
   r2.AddUnique(Rect2(Point2(1, 2), Point2(3, 4)));

   RegionTools2 tools;
   tools.ForEachPlane(r2, [=](Region1 const& r1, RegionTools2::PlaneInfo const& pi) {
      for (const auto &c : r1) {
         LOG_() << c << " " << pi.reduced_value;
      }
   });
}

TEST(RegionTools2, ForEachEdge) {
   Region2 r2;
   r2.AddUnique(Rect2(Point2(1, 2), Point2(3, 4)));

   RegionTools2 tools;
   tools.ForEachEdge(r2, [=](csg::EdgeInfo2 const& edge_info) {
      LOG_() << edge_info.min << " " << edge_info.max << " " << edge_info.normal;
   });
}

TEST(RegionTools2, Inset) {
   Region2 r2;
   r2.AddUnique(Rect2(Point2(1, 2), Point2(3, 4)));

   RegionTools2 tools;
   Region2f inset = tools.Inset(r2, 0.1f);
   for (const auto &c : inset) {
      LOG_() << c;
   }
}

TEST(RegionTools3, ForEachPlane) {
   Region3 r3;
   r3.AddUnique(Cube3(Point3(0, 1, 2), Point3(100, 101, 102)));

   RegionTools3 tools;
   tools.ForEachPlane(r3, [=](Region2 const& r2, RegionTools3::PlaneInfo const& pi) {
      for (const auto &c : r2) {
         LOG_() << c << " " << pi.reduced_value;
      }
   });
}

TEST(RegionTools3, ForEachEdge) {
   Region3 r3;
   r3.AddUnique(Cube3(Point3(0, 1, 2), Point3(100, 101, 102)));

   RegionTools3 tools;
   int i = 0;
   tools.ForEachEdge(r3, [&](csg::EdgeInfo3 const& edge_info) {
      LOG_() << std::setw(3) << i++ << " " << edge_info.min << " " << edge_info.max << " " << edge_info.normal;
   });
}

TEST(RegionTools3, Inset) {
   Region3 r3;
   r3.AddUnique(Cube3(Point3(0, 0, 0), Point3(1, 1, 1)));

   RegionTools3 tools;
   Region3f inset = tools.Inset(r3, 0.01f);
   for (const auto &c : inset) {
      LOG_() << c;
   }
   LOG_() << "---";
   tools.ForEachPlane(r3, [=](Region2 const& r2, RegionTools3::PlaneInfo const& pi) {
      Region2f inset = RegionTools2().Inset(r2, 0.1f);
      for (const auto &c : inset) {
         LOG_() << RegionToolsTraits<float, 3>::ExpandCube(c, ToFloat(pi));
      }
   });
}


TEST(RegionTools3, InsetConcave) {
   Region3 r3;
   r3.Add(Cube3(Point3(-10, -2, -3), Point3(10, 2, 3)));
   r3.Add(Cube3(Point3(-1, -20, -3), Point3(1, 20, 3)));
   r3.Add(Cube3(Point3(-1, -2, -30), Point3(1, 2, 30)));

   RegionTools3 tools;
   Region3f inset = tools.Inset(r3, 1);
   for (const auto &c : inset) {
      LOG_() << c;
   }
   LOG_() << "---";
   tools.ForEachPlane(r3, [=](Region2 const& r2, RegionTools3::PlaneInfo const& pi) {
      Region2f inset = RegionTools2().Inset(r2, 1);
      for (const auto &c : inset) {
         LOG_() << RegionToolsTraits<float, 3>::ExpandCube(c, ToFloat(pi));
      }
   });
}

#endif

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
