#ifndef _RADIANT_SIMULATION_SIMULATION_H
#define _RADIANT_SIMULATION_SIMULATION_H

#include "om/om.h"
#include "lib/lua/bind.h"
#include "physics/octtree.h"
#include "platform/utils.h"
#include "platform/random.h"

#include "interfaces.h"
#include "dm/store.h"
#include "core/guard.h"
#include "libjson.h"
#include "physics/namespace.h"
#include "lib/json/node.h"
#include "lib/rpc/forward_defines.h"
#include "lib/lua/lua.h"

IN_RADIANT_OM_NAMESPACE(
   class Entity;
)

IN_RADIANT_PHYSICS_NAMESPACE(
   class OctTree;
)

BEGIN_RADIANT_SIMULATION_NAMESPACE

class Job;
class Task;
class Event;
class WorkerScheduler;
class BuildingScheduler;
class PathFinder;
class FollowPath;
class GotoLocation;
class Path;

class Simulation : public SimulationInterface {
public:
   Simulation();
   ~Simulation();

   om::EntityPtr CreateEntity();
   om::EntityPtr Simulation::GetEntity(dm::ObjectId id);
   void DestroyEntity(dm::ObjectId id);

   static Simulation& GetInstance();
   void CreateNew() override;
   void Save(std::string id) override;
   void Load(std::string id) override;

   /* New object model stuff goes here */

   /* End of new object model stuff */
   void AddTask(std::shared_ptr<Task> task);
   void AddJob(std::shared_ptr<Job> job);

   bool ProcessMessage(const ::radiant::tesseract::protocol::Request& msg, protocol::SendQueuePtr queue);
   void EncodeUpdates(protocol::SendQueuePtr queue, ClientState& cs) override;
   void Step(platform::timer &timer, int interval) override;
   void Idle(platform::timer &timer) override;

   om::EntityPtr GetRootEntity();
   phys::OctTree &GetOctTree();
   dm::Store& GetStore();
   lua::ScriptHost& GetScript();

   WorkerScheduler* GetWorkerScheduler();
   BuildingScheduler* GetBuildingScehduler(dm::ObjectId id);

private:
   void PostCommand(tesseract::protocol::PostCommandRequest const& request);
   void EncodeDebugShapes(protocol::SendQueuePtr queue);
   void PushServerRemoteObjects(protocol::SendQueuePtr queue);
   void ProcessTaskList(platform::timer &timer);
   void ProcessJobList(platform::timer &timer);
   void OnObjectAllocated(dm::ObjectPtr obj);
   void OnObjectDestroyed(dm::ObjectId id);

   void StepPathFinding();

   void TraceEntity(om::EntityPtr e);
   void ComponentAdded(om::EntityRef e, dm::ObjectType type, std::shared_ptr<dm::Object> component);
   void TraceTargetTables(om::TargetTablesPtr tables);
   void UpdateTargetTables(int now, int interval);
   void SendReply(tesseract::protocol::PostCommandReply const& reply);
   void InitializeModules();
   void InitDataModel();

private:
   static Simulation*                           singleton_;

private:
   dm::Store                                             store_;
   core::Guard                                             guards_;
   std::vector<std::pair<dm::ObjectId, dm::ObjectType>>  allocated_;
   std::vector<dm::ObjectId>                             destroyed_;
   std::unique_ptr<phys::OctTree>                     octtree_;
   std::unique_ptr<lua::ScriptHost>                      scriptHost_;

   // Good stuff down here.

   // Terrain Infrastructure
   int                                          now_;
   int                                          lastNow_;
   bool                                         _showDebugNodes;
   bool                                         _singleStepPathFinding;
   std::list<std::weak_ptr<Job>>                jobs_;
   std::list<std::weak_ptr<Task>>               tasks_;
   std::vector<om::TargetTablesRef>             targetTables_;   
   std::unordered_map<std::string, luabind::object>   routes_;
   std::vector<std::pair<std::string, std::string>>   serverRemoteObjects_;

   luabind::object                              p1_;
   luabind::object                              game_;
   luabind::object                              game_api_;
   std::map<dm::ObjectId, om::EntityPtr>        entityMap_;

   rpc::SessionPtr             session_;
   rpc::CoreReactorPtr         core_reactor_;
   rpc::TraceObjectRouterPtr   trace_router_;
   rpc::ProtobufReactorPtr     protobuf_reactor_;
   std::vector<tesseract::protocol::Update>  buffered_updates_;

   dm::StreamerPtr      streamer_;
};

END_RADIANT_SIMULATION_NAMESPACE

#endif //  _RADIANT_SIMULATION_SIMULATION_H
