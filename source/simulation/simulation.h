#ifndef _RADIANT_SIMULATION_SIMULATION_H
#define _RADIANT_SIMULATION_SIMULATION_H

#define BOOST_ASIO_PLACEHOLDERS_HPP
#include <boost/asio.hpp>

#include "om/om.h"
#include "lib/lua/bind.h"
#include "physics/octtree.h"
#include "platform/utils.h"
#include "dm/store.h"
#include "core/guard.h"
#include "libjson.h"
#include "physics/namespace.h"
#include "lib/json/node.h"
#include "lib/rpc/forward_defines.h"
#include "lib/lua/lua.h"
#include "namespace.h"
#include "protocols/tesseract.pb.h"
#include "lib/perfmon/store.h"
#include "lib/perfmon/timeline.h"
#include "protocol.h"

using boost::asio::ip::tcp;
namespace boost {
   namespace program_options {
      class options_description;
   }
}

IN_RADIANT_OM_NAMESPACE(
   class Entity;
)

IN_RADIANT_PHYSICS_NAMESPACE(
   class OctTree;
)

BEGIN_RADIANT_SIMULATION_NAMESPACE

class RemoteClient;
class Job;
class Task;
class Event;
class WorkerScheduler;
class BuildingScheduler;
class PathFinder;
class FollowPath;
class GotoLocation;
class Path;

class Simulation
{
public:
   Simulation();
   ~Simulation();

   om::EntityPtr CreateEntity();
   om::EntityPtr Simulation::GetEntity(dm::ObjectId id);
   void DestroyEntity(dm::ObjectId id);

   void CreateNew();
   void Save(std::string id);
   void Load(std::string id);

   void Run(tcp::acceptor* acceptor, boost::asio::io_service* io_service);

   /* New object model stuff goes here */

   /* End of new object model stuff */
   void AddTask(std::shared_ptr<Task> task);
   void AddJob(std::shared_ptr<Job> job);
   void AddJobForEntity(om::EntityPtr entity, PathFinderPtr job);

   bool ProcessMessage(std::shared_ptr<RemoteClient> c, const tesseract::protocol::Request& msg);
   void EncodeUpdates(protocol::SendQueuePtr queue);

   om::EntityPtr GetRootEntity();
   phys::OctTree &GetOctTree();
   dm::Store& GetStore();
   lua::ScriptHost& GetScript();
   perfmon::Store& GetPerfmonCounters();
   float GetBaseWalkSpeed() const;
   int GetStepInterval() const;

   WorkerScheduler* GetWorkerScheduler();
   BuildingScheduler* GetBuildingScehduler(dm::ObjectId id);

private:
   void PostCommand(tesseract::protocol::PostCommandRequest const& request);
   void EncodeDebugShapes(protocol::SendQueuePtr queue);
   void ProcessTaskList();
   void ProcessJobList();
   void StepPathFinding();
   rpc::ReactorDeferredPtr StartTaskManager();
   rpc::ReactorDeferredPtr StartPerformanceCounterPush();
   void PushPerformanceCounters();
   void UpdateGameState();
   void UpdateCollisions();

   void SendReply(tesseract::protocol::PostCommandReply const& reply);
   void InitializeModules();
   void InitDataModel();
   void main(); // public for the server.  xxx - there's a better way to factor this between the server and the in-proc listen server
   void Mainloop();
   void Idle();
   void SendClientUpdates();
            
   //void OnCellHover(render3d::RendererInterface *renderer, int x, int y, int z);
   //void on_keyboard_pressed(render3d::RendererInterface *renderer, const render3d::keyboard_event &e);

   void ReadClientMessages();
   void FireLuaTraces();
   void LuaGC();

   void start_accept();
   void handle_accept(std::shared_ptr<tcp::socket> s, const boost::system::error_code& error);
   void SendUpdates(std::shared_ptr<RemoteClient> c);
   void EncodeBeginUpdate(std::shared_ptr<RemoteClient> c);
   void EncodeEndUpdate(std::shared_ptr<RemoteClient> c);
   void EncodeServerTick(std::shared_ptr<RemoteClient> c);
   void EncodeUpdates(std::shared_ptr<RemoteClient> c);

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
   bool                                         debug_navgrid_enabled_;
   csg::Point3                                  debug_navgrid_point_;

   std::unordered_map<dm::ObjectId, EntityJobSchedulerPtr>  entity_jobs_schedulers_;
   std::list<std::weak_ptr<Job>>                jobs_;
   std::list<std::weak_ptr<Task>>               tasks_;
   std::unordered_map<std::string, luabind::object>   routes_;

   luabind::object                              p1_;
   luabind::object                              game_;
   luabind::object                              game_api_;
   std::map<dm::ObjectId, om::EntityPtr>        entityMap_;

   rpc::SessionPtr             session_;
   rpc::CoreReactorPtr         core_reactor_;
   rpc::TraceObjectRouterPtr   trace_router_;
   rpc::ProtobufReactorPtr     protobuf_reactor_;
   std::vector<tesseract::protocol::Update>  buffered_updates_;

   dm::TracerSyncPtr       object_model_traces_;
   dm::TracerSyncPtr       pathfinder_traces_;
   dm::TracerBufferedPtr   lua_traces_;
   dm::StoreTracePtr       store_trace_;

   int            sequence_number_;
   bool           paused_;
   bool           noidle_;
   boost::asio::io_service*            _io_service;
   tcp::acceptor*                      _tcp_acceptor;
   std::vector<std::shared_ptr<RemoteClient>>   _clients;
   platform::timer                     game_loop_timer_;
   platform::timer                     net_send_timer_;
   int                                 game_tick_interval_;
   int                                 net_send_interval_;
   float                               base_walk_speed_;
   bool                                profile_next_lua_update_;
   rpc::ReactorDeferredPtr             task_manager_deferred_;
   rpc::ReactorDeferredPtr             perf_counter_deferred_;
   perfmon::Timeline                   perf_timeline_;
   perfmon::Store                      perf_counters_;
   platform::timer                     next_counter_push_;
   core::Guard                         on_frame_end_guard_;
   om::ErrorBrowserPtr                 error_browser_;
   om::EntityPtr                       root_entity_;
};

END_RADIANT_SIMULATION_NAMESPACE

#endif //  _RADIANT_SIMULATION_SIMULATION_H
