#ifndef _RADIANT_SIMULATION_SIMULATION_H
#define _RADIANT_SIMULATION_SIMULATION_H

#define BOOST_ASIO_PLACEHOLDERS_HPP
#include <boost/asio.hpp>

#include "om/om.h"
#include "core/static_string.h"
#include "lib/lua/bind.h"
#include "physics/octtree.h"
#include "physics/free_motion.h"
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
#include "core/singleton.h"
#include "protocol.h"

using boost::asio::ip::tcp;

IN_RADIANT_OM_NAMESPACE(
   class Entity;
)

IN_RADIANT_PHYSICS_NAMESPACE(
   class OctTree;
)

#define MEASURE_TASK_TIME(perf, category)  perfmon::TimelineCounterGuard taskman_timer_ ## __LINE__ (perf, category);

BEGIN_RADIANT_SIMULATION_NAMESPACE

class RemoteClient;
class Job;
class Task;
class Event;
class WorkerScheduler;
class BuildingScheduler;
class PathFinder;
class FollowPath;
class Path;

class Simulation : public core::Singleton<Simulation>
{
public:
   Simulation();
   ~Simulation();

   om::DataStoreRef Simulation::AllocDatastore();
   om::EntityPtr Simulation::GetEntity(dm::ObjectId id);
   void DestroyEntity(dm::ObjectId id);
   void RemoveDataStoreFromMap(dm::ObjectId id);
   void Run(tcp::acceptor* acceptor, boost::asio::io_service* io_service);

   /* New object model stuff goes here */

   /* End of new object model stuff */
   void AddTask(std::shared_ptr<Task> task);
   void AddJob(std::shared_ptr<Job> job);
   void AddJobForEntity(om::EntityPtr entity, PathFinderPtr job);
   void RemoveJobForEntity(om::EntityPtr entity, PathFinderPtr job);

   bool ProcessMessage(std::shared_ptr<RemoteClient> c, const tesseract::protocol::Request& msg);
   void EncodeUpdates(protocol::SendQueuePtr queue);

   om::EntityPtr GetRootEntity();
   phys::OctTree &GetOctTree();
   dm::Store& GetStore();
   lua::ScriptHost& GetScript();
   float GetBaseWalkSpeed() const;
   int GetGameTickInterval() const;
   bool GetEnableJobLogging() const;
   perfmon::Timeline& GetOverviewPerfTimeline();
   perfmon::Timeline& GetJobsPerfTimeline();

   WorkerScheduler* GetWorkerScheduler();
   BuildingScheduler* GetBuildingScehduler(dm::ObjectId id);

private:
   void PostCommandRequest(tesseract::protocol::PostCommandRequest const& msg);
   void FinishedUpdate(tesseract::protocol::FinishedUpdate const& msg);
   void EncodeDebugShapes(protocol::SendQueuePtr queue);
   void ProcessTaskList();
   void ProcessJobList();
   void StepPathFinding();
   rpc::ReactorDeferredPtr StartTaskManager();
   rpc::ReactorDeferredPtr StartPerformanceCounterPush();
   void PushPerformanceCounters();
   void UpdateGameState();

   void SendReply(tesseract::protocol::PostCommandReply const& reply);
   void InitializeModules();
   void main(); // public for the server.  xxx - there's a better way to factor this between the server and the in-proc listen server
   void Mainloop();
   void Idle();
   void SendClientUpdates(bool throttle);
            
   //void OnCellHover(render3d::RendererInterface *renderer, int x, int y, int z);
   //void on_keyboard_pressed(render3d::RendererInterface *renderer, const render3d::keyboard_event &e);

   void ReadClientMessages();
   void FireLuaTraces();
   void LuaGC();

   void start_accept();
   void handle_accept(std::shared_ptr<tcp::socket> s, const boost::system::error_code& error);
   void SendUpdates(std::shared_ptr<RemoteClient> c, bool throttle);
   void EncodeBeginUpdate(std::shared_ptr<RemoteClient> c);
   void EncodeEndUpdate(std::shared_ptr<RemoteClient> c);
   void EncodeServerTick(std::shared_ptr<RemoteClient> c);
   void EncodeUpdates(std::shared_ptr<RemoteClient> c);
   void FlushStream(std::shared_ptr<RemoteClient> c);

   void Save(boost::filesystem::path const& savedir);
   void Load();
   void FinishLoadingGame();
   rpc::ReactorDeferredPtr BeginLoad(boost::filesystem::path const& savedir);
   void Reset();

   void OneTimeIninitializtion();
   void Initialize();
   void InitializeDataObjects();
   void InitializeDataObjectTraces();
   void InitializeGameObjects();

   void Shutdown();
   void ShutdownGameObjects();
   void ShutdownDataObjects();
   void ShutdownDataObjectTraces();
   void ShutdownLuaObjects();

   void CreateGame();
   void CreateFreeMotionTrace(om::MobPtr mob);
   void LogJobPerfCounters(perfmon::Frame* frame);
   std::string GetProgressForJob(core::StaticString name) const;
   void ForEachPathFinder(std::function<void(PathFinderPtr const&)> cb);

private:
   struct FreeMotionTaskMapEntry {
      ApplyFreeMotionTaskPtr     task;
      dm::TracePtr               trace;
   };

   typedef std::unordered_map<dm::ObjectId, FreeMotionTaskMapEntry> FreeMotionTaskMap;

private:
   std::unique_ptr<dm::Store>                            store_;
   std::unique_ptr<phys::OctTree>                        octtree_;
   std::unique_ptr<phys::FreeMotion>                     freeMotion_;
   std::unique_ptr<phys::WaterTightRegionBuilder>        waterTightRegionBuilder_;
   std::unique_ptr<lua::ScriptHost>                      scriptHost_;

   // Good stuff down here.

   // Terrain Infrastructure
   int                                          now_;
   bool                                         _showDebugNodes;
   bool                                         _singleStepPathFinding;
   std::string                                  debug_navgrid_mode_;
   csg::Point3                                  debug_navgrid_point_;
   om::EntityRef                                debug_navgrid_pawn_;

   std::unordered_map<dm::ObjectId, EntityJobSchedulerPtr>  entity_jobs_schedulers_;
   std::list<std::weak_ptr<Job>>                jobs_;
   std::list<std::weak_ptr<Task>>               tasks_;

   luabind::object                              radiant_;
   std::unordered_map<dm::ObjectId, om::EntityPtr>        entityMap_;
   std::unordered_map<dm::ObjectId, om::DataStorePtr>     datastoreMap_;

   rpc::SessionPtr             session_;
   rpc::CoreReactorPtr         core_reactor_;
   rpc::TraceObjectRouterPtr   trace_router_;
   rpc::ProtobufReactorPtr     protobuf_reactor_;
   std::vector<tesseract::protocol::Update>  buffered_updates_;

   dm::TracerSyncPtr       object_model_traces_;
   dm::TracerBufferedPtr   pathfinder_traces_;
   dm::TracerBufferedPtr   lua_traces_;
   dm::StoreTracePtr       store_trace_;

   bool           waiting_for_client_;
   bool           noidle_;
   boost::asio::io_service*            _io_service;
   tcp::acceptor*                      _tcp_acceptor;
   std::vector<std::shared_ptr<RemoteClient>>   _clients;
   platform::timer                     game_loop_timer_;
   platform::timer                     net_send_timer_;
   int                                 game_tick_interval_;
   int                                 net_send_interval_;
   float                               base_walk_speed_;
   rpc::ReactorDeferredPtr             task_manager_deferred_;
   rpc::ReactorDeferredPtr             perf_counter_deferred_;
   rpc::ReactorDeferredPtr             load_progress_deferred_;
   rpc::LuaModuleRouterPtr             luaModuleRouter_;
   rpc::LuaObjectRouterPtr             luaObjectRouter_;
   rpc::TraceObjectRouterPtr           traceObjectRouter_;
   perfmon::Timeline                   perf_timeline_;
   perfmon::Timeline                   perf_jobs_;
   bool                                enable_job_logging_;
   platform::timer                     log_jobs_timer_;
   platform::timer                     lua_memory_timer_;
   platform::timer                     next_counter_push_;
   core::Guard                         on_frame_end_guard_;
   core::Guard                         jobs_perf_guard_;
   om::ErrorBrowserPtr                 error_browser_;
   om::EntityPtr                       root_entity_;
   om::ModListPtr                      modList_;
   om::ClockPtr                        clock_;
   float                               game_speed_;
   FreeMotionTaskMap                   freeMotionTasks_;
   bool                                begin_loading_;
   boost::filesystem::path             load_saveid_;
   std::vector<std::function<void()>>  _bottomLoopFns;
   int                                 _sequenceNumber;
   std::function<om::DataStoreRef(int storeId)> _allocDataStoreFn;
};

END_RADIANT_SIMULATION_NAMESPACE

#endif //  _RADIANT_SIMULATION_SIMULATION_H
