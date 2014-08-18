#include "pch.h"
#include "follow_path.h"
#include "om/entity.h"
#include "om/components/mob.ridl.h"
#include "simulation/simulation.h"
#include "csg/util.h" // xxx: should be csg/csg.h
#include "lib/lua/script_host.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

#define FP_LOG(level)      LOG_CATEGORY(simulation.follow_path, level, GetName())

std::ostream& simulation::operator<<(std::ostream& os, FollowPath const& o)
{
   return os << "[FollowPath ...]";
}

FollowPath::FollowPath(Simulation& sim, om::EntityRef e, float speed, std::shared_ptr<Path> path, float stopDistance, luabind::object unsafe_arrived_cb) :
   Task(sim, "follow path"),
   entity_(e),
   path_(path),
   pursuing_(0),
   speed_(speed),
   stopDistance_(stopDistance),
   stopIndex_(-1)
{
   path_->PrunePoints();

   om::EntityPtr entity = entity_.lock();
   if (entity) {
      om::MobPtr mob = entity->AddComponent<om::Mob>();
      csg::Point3f startLocation = mob->GetWorldLocation();
      pursuing_ = CalculateStartIndex(csg::ToClosestInt(startLocation));
      stopIndex_ = CalculateStopIndex(startLocation, path_->GetPoints(), path_->GetDestinationPointOfInterest(), stopDistance_);
   }

   lua_State* cb_thread = lua::ScriptHost::GetCallbackThread(unsafe_arrived_cb.interpreter());  
   arrived_cb_ = luabind::object(cb_thread, unsafe_arrived_cb);

   Report("constructor");
}

FollowPath::~FollowPath()
{
   Report("destroying pathfinder");
}

int FollowPath::CalculateStartIndex(csg::Point3 const& startGridLocation) const
{
   std::vector<csg::Point3> points = path_->GetPoints();

   if (points.size() > 1 && points.front() == startGridLocation) {
      // skip the first point so that we don't seek the center of the current voxel before starting on the path
      return 1;
   } else {
      return 0;
   }
}

int FollowPath::CalculateStopIndex(csg::Point3f const& startLocation, std::vector<csg::Point3> const& points, csg::Point3 const& pointOfInterest, float stopDistance) const
{
   int lastIndex = points.size()-1;

   if (stopDistance == 0) {
      return lastIndex;
   }

   csg::Point3 previousPoint = pointOfInterest;
   csg::Point3 currentPoint;
   float distance = 0;
   int index = lastIndex;
   int i;

   // Distance is the path distance to the POI, not the shortest euclidian distance
   // because the path may curl upon itself
   for (i = lastIndex; i >= 0; --i) {
      currentPoint = points[i];
      distance += currentPoint.DistanceTo(previousPoint);
      if (distance > stopDistance) {
         // return the last index where the distance was closer than the stopDistance
         break;
      }
      index = i;
      previousPoint = currentPoint;
   }

   // If the entire travelled path is still less than the stopDistance, we don't need to move at all
   if (index == 0) {
      if (startLocation.DistanceTo(csg::ToFloat(pointOfInterest)) <= stopDistance) {
         index = -1;
      }
   }

   return index;
}

static float angle(csg::Point3f const& v)
{
   ASSERT(v.x != 0 || v.z != 0);
   csg::Point3f const forward(0, 0, -1);
   float angle = (float)(atan2(-v.z, v.x) - atan2(-forward.z, forward.x));
   if (angle < 0)  {
      angle += 2 * csg::k_pi;
   }
   return angle;
}

bool FollowPath::Work(platform::timer const& timer)
{
   Report("running");

   auto entity = entity_.lock();
   if (!entity) {
      return false;
   }

   float moveDistance = speed_ * GetSim().GetBaseWalkSpeed();
   std::vector<csg::Point3> const& points = path_->GetPoints();
   om::MobPtr mob = entity->AddComponent<om::Mob>();

   while (!Arrived() && !Obstructed() && moveDistance > 0)  {
      csg::Point3f const location = mob->GetLocation();
      csg::Point3f const goal = csg::ToFloat(points[pursuing_]);
      csg::Point3f const goalVector = goal - location;
      float const goalDistance = goalVector.Length();

      if (goalDistance < moveDistance || goalDistance == 0) {
         mob->MoveTo(goal);
         moveDistance -= goalDistance;
         pursuing_++;
      } else {
         mob->MoveTo(location + goalVector * (moveDistance / goalDistance));
         moveDistance = 0;
      }

      if (goalVector.x != 0 || goalVector.z != 0) {
         // angle is undefined when goal and location are at the same x,z coordinates
         mob->TurnTo(angle(goalVector) * 180 / csg::k_pi);
      }
   }

   if (Arrived()) {
      Report("arrived!");
      if (arrived_cb_.is_valid()) {
         try {
            arrived_cb_();
         } catch (std::exception const& e) {
            LUA_LOG(1) << "exception delivering solved cb: " << e.what();
         }
      }
      return false;
   }

   return true;
}

bool FollowPath::Arrived() const
{
   ASSERT(path_);

   if (pursuing_ < stopIndex_) {
      return false;
   }

   // Don't travel all the way to stopIndex_ if not necessary.
   // This becomes important if we prune points out of the path so they they are not adjacent.
   // We'll want to prune so that entities stop zig-zagging to their destination
   // which looks increasingly odd as the entities get bigger (bosses).
   // If stopDistance_ == 0 then make sure we finish pursuing all points on the path.
   // This is important when the entity and poi are at the same location and we're trying
   // to move to a point in the adjacent region (e.g. to pick up the item).
   if (pursuing_ == stopIndex_) {
      om::EntityPtr entity = entity_.lock();
      if (entity) {
         om::MobPtr mob = entity->AddComponent<om::Mob>();
         csg::Point3f location = mob->GetLocation();

         if (stopDistance_ > 0) {
            csg::Point3f poi = csg::ToFloat(path_->GetDestinationPointOfInterest());
            float distance = location.DistanceTo(poi);
            return distance <= stopDistance_;
         } else {
            std::vector<csg::Point3> const& points = path_->GetPoints();
            csg::Point3f goal = csg::ToFloat(points[stopIndex_]);
            return location == goal;
         }
      }
   }

   return true;
}

bool FollowPath::Obstructed() const
{
   // We'll need to implement this to prevent entities from walking through geometry that
   // didn't exist when the path was created.
   return false;
}

void FollowPath::Stop()
{
   Report("stopping");
   entity_.reset();
}

void FollowPath::Report(std::string const& msg) const
{
   auto entity = entity_.lock();
   if (entity) {
      auto const& points = path_->GetPoints();
      if (!points.empty()) {
         auto start = points.front();
         auto end = points.back();
         auto location = entity->AddComponent<om::Mob>()->GetWorldLocation();
         FP_LOG(5) << msg << " (entity:" << entity->GetObjectId() << " " << start << " -> " << end << " currently at " << location << ")";
      } else {
         FP_LOG(5) << msg << " (entity:" << entity->GetObjectId() << " path has no points)";
      }
   }
}
