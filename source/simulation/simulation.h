#ifndef _RADIANT_SIMULATION_SIMULATION_H
#define _RADIANT_SIMULATION_SIMULATION_H

#include "om/om.h"
#include "physics/octtree.h"
#include "platform/utils.h"
#include "platform/random.h"

#include "interfaces.h"
#include "dm/store.h"
#include "dm/guard.h"
#include "libjson.h"
#include "physics/namespace.h"
#include "radiant_json.h"

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
class Task;
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
   void AddTask(std::shared_ptr<Task> task);
   void AddJob(std::shared_ptr<Job> job);

   void ProcessCommand(const ::radiant::tesseract::protocol::Cmd &cmd) override;
   void ProcessCommand(::google::protobuf::RepeatedPtrField<tesseract::protocol::Reply >* replies, const ::radiant::tesseract::protocol::Command &command) override;
   // void ProcessCommandList(const ::radiant::tesseract::protocol::command_list &cmd_list) override;

   bool ProcessMessage(const ::radiant::tesseract::protocol::Request& msg, protocol::SendQueuePtr queue);
   void EncodeUpdates(protocol::SendQueuePtr queue, ClientState& cs) override;
   void Step(platform::timer &timer, int interval) override;
   void Idle(platform::timer &timer) override;

   void SendCommandReply(::radiant::tesseract::protocol::Reply* reply);

   om::EntityPtr GetRootEntity();
   Physics::OctTree &GetOctTree();
   dm::Store& GetStore();
   ScriptHost& GetScript() { return *scriptHost_; }

   WorkerScheduler* GetWorkerScheduler();
   BuildingScheduler* GetBuildingScehduler(dm::ObjectId id);

private:
   void FetchObject(tesseract::protocol::FetchObjectRequest const& request, tesseract::protocol::FetchObjectReply* reply);
   void PostCommand(tesseract::protocol::PostCommandRequest const& request, tesseract::protocol::PostCommandReply* reply);
   void ScriptCommand(tesseract::protocol::ScriptCommandRequest const& request, tesseract::protocol::ScriptCommandReply* reply);
   void EncodeDebugShapes(protocol::SendQueuePtr queue);
   void ProcessJobList(platform::timer &timer);
   void OnObjectAllocated(dm::ObjectPtr obj);
   void OnObjectDestroyed(dm::ObjectId id);

   typedef std::unordered_map<std::string, std::function<std::string (std::string const& cmd)>> NativeCommandHandlers; 
   std::string ToggleDebugShapes(std::string const& cmd);
   std::string ToggleStepPathFinding(std::string const& cmd);
   std::string StepPathFinding(std::string const& cmd);

   void TraceEntity(om::EntityPtr e);
   void ComponentAdded(om::EntityRef e, dm::ObjectType type, std::shared_ptr<dm::Object> component);
   void UpdateAuras(int now);
   void TraceAura(om::AuraListRef auraList, om::AuraPtr aura);
   void TraceTargetTables(om::TargetTablesPtr tables);
   void UpdateTargetTables(int now, int interval);
   void HandleRouteRequest(luabind::object ctor, JSONNode const& query, std::string const& postdata, tesseract::protocol::PostCommandReply* response);
   void LoadModuleInitScript(json::ConstJsonObject const& block);
   void LoadModuleRoutes(std::string const& modname, json::ConstJsonObject const& block);

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
   dm::Guard                                             guards_;
   std::vector<std::pair<dm::ObjectId, dm::ObjectType>>  allocated_;
   std::vector<dm::ObjectId>                             destroyed_;
   std::unique_ptr<Physics::OctTree>                     octtree_;
   std::unique_ptr<ScriptHost>                           scriptHost_;

   // Good stuff down here.

   // Terrain Infrastructure
   int                                          now_;
   int                                          lastNow_;
   bool                                         _showDebugNodes;
   bool                                         _singleStepPathFinding;
   std::list<std::weak_ptr<Job>>                jobs_;
   std::list<std::weak_ptr<Task>>               tasks_;
   std::vector<AuraListEntry>                   auras_;
   std::vector<om::TargetTablesRef>             targetTables_;   
   lua_State* L_;
   std::unordered_map<std::string, luabind::object>   routes_;
};

END_RADIANT_SIMULATION_NAMESPACE

#endif //  _RADIANT_SIMULATION_SIMULATION_H
