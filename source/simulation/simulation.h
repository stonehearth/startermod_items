#ifndef _RADIANT_SIMULATION_SIMULATION_H
#define _RADIANT_SIMULATION_SIMULATION_H

#include "om/om.h"
#include "physics/octtree.h"
#include "platform/utils.h"
#include "platform/random.h"

#include "interfaces.h"
#include "dm/store.h"
#include "dm/guard_set.h"
#include "libjson.h"
#include "physics/namespace.h"

// Forward Declarations
IN_RADIANT_OM_NAMESPACE(
   class Entity;
)

IN_RADIANT_PHYSICS_NAMESPACE(
   class OctTree;
)

BEGIN_RADIANT_SIMULATION_NAMESPACE

class ScriptHost;
class Job;
class Event;
class WorkerScheduler;
class BuildingScheduler;
class MultiPathFinder;
class PathFinder;
class FollowPath;
class GotoLocation;
class Path;

class Simulation : public SimulationInterface {
public:
   Simulation(lua_State* L);
   ~Simulation();

   static Simulation& GetInstance();
   void CreateNew() override;
   void Save(std::string id) override;
   void Load(std::string id) override;

   /* New object model stuff goes here */

   /* End of new object model stuff */
   std::shared_ptr<MultiPathFinder> CreateMultiPathFinder(std::string name);
   std::shared_ptr<PathFinder> CreatePathFinder(std::string name, om::EntityRef entity, luabind::object solved, luabind::object dst_filter); // xxx: should be an entity ref.  don't keep the entity alive!
   std::shared_ptr<FollowPath> CreateFollowPath(om::EntityRef entity, float speed, std::shared_ptr<Path> path, float close_to_distance);
   std::shared_ptr<GotoLocation> CreateGotoLocation(om::EntityRef entity, float speed, const math3d::point3& location, float close_to_distance);
   std::shared_ptr<GotoLocation> CreateGotoEntity(om::EntityRef entity, float speed, om::EntityRef target, float close_to_distance);

   void ProcessCommand(const ::radiant::tesseract::protocol::Cmd &cmd) override;
   void ProcessCommand(::google::protobuf::RepeatedPtrField<tesseract::protocol::Reply >* replies, const ::radiant::tesseract::protocol::Command &command) override;
   // void ProcessCommandList(const ::radiant::tesseract::protocol::command_list &cmd_list) override;

   void DoAction(const tesseract::protocol::DoAction& msg, protocol::SendQueuePtr queue) override; // xxx: die , die , die
   bool ProcessMessage(const ::radiant::tesseract::protocol::Request& msg, protocol::SendQueuePtr queue);
   void EncodeUpdates(protocol::SendQueuePtr queue, ClientState& cs) override;
   void Step(platform::timer &timer, int interval) override;
   void Idle(platform::timer &timer) override;

   void SendCommandReply(::radiant::tesseract::protocol::Reply* reply);

   om::EntityPtr GetRootEntity();
   Physics::OctTree &GetOctTree();
   dm::Store& GetStore();
   ScriptHost& GetScript() { return *scripts_; }

   WorkerScheduler* GetWorkerScheduler();
   BuildingScheduler* GetBuildingScehduler(om::EntityId id);

private:
   void FetchObject(tesseract::protocol::FetchObjectRequest const& request, tesseract::protocol::FetchObjectReply* reply);
   void PostCommand(tesseract::protocol::PostCommandRequest const& request, tesseract::protocol::PostCommandReply* reply);
   void EncodeDebugShapes(protocol::SendQueuePtr queue);
   void ProcessJobList(int now, platform::timer &timer);
   void OnObjectAllocated(dm::ObjectPtr obj);
   void OnObjectDestroyed(dm::ObjectId id);

   typedef std::unordered_map<std::string, std::function<std::string (const tesseract::protocol::DoAction& msg)>> NativeCommandHandlers; 
   std::string ToggleDebugShapes(const tesseract::protocol::DoAction& msg);
   std::string ToggleStepPathFinding(const tesseract::protocol::DoAction& msg);
   std::string StepPathFinding(const tesseract::protocol::DoAction& msg);

   void TraceEntity(om::EntityPtr e);
   void ComponentAdded(om::EntityRef e, dm::ObjectType type, std::shared_ptr<dm::Object> component);
   void UpdateAuras(int now);
   void TraceAura(om::AuraListRef auraList, om::AuraPtr aura);
   void TraceTargetTables(om::TargetTablesPtr tables);
   void UpdateTargetTables(int now, int interval);

private:
   static Simulation*                           singleton_;

private:
   struct AuraListEntry {
      AuraListEntry() {}
      AuraListEntry(om::AuraListRef l, om::AuraRef a) : list(l), aura(a) { }

      om::AuraRef aura;
      om::AuraListRef list;
   };   

private:
   NativeCommandHandlers                                 commands_;
   dm::Store                                             store_;
   dm::GuardSet                                          guards_;
   std::vector<std::pair<dm::ObjectId, dm::ObjectType>>  allocated_;
   std::vector<dm::ObjectId>                             destroyed_;
   std::unique_ptr<Physics::OctTree>                     octtree_;
   std::unique_ptr<ScriptHost>                           scripts_;

   // Good stuff down here.

   // Terrain Infrastructure
   int                                          now_;
   int                                          lastNow_;
   bool                                         _showDebugNodes;
   bool                                         _singleStepPathFinding;
   std::list<std::weak_ptr<Job>>                _pathFinders;
   std::list<std::weak_ptr<Job>>                _loopTasks;
   std::vector<AuraListEntry>                   auras_;
   std::vector<om::TargetTablesRef>             targetTables_;   
   lua_State* L_;
};

END_RADIANT_SIMULATION_NAMESPACE

#endif //  _RADIANT_SIMULATION_SIMULATION_H
