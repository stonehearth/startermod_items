#include "pch.h"
#include <sstream>
#include <boost/algorithm/string.hpp>
#include "core/config.h"
#include "resources/manifest.h"
#include "radiant_exceptions.h"
#include "platform/utils.h"
#include "simulation.h"
#include "jobs/bfs_path_finder.h"
#include "jobs/a_star_path_finder.h"
#include "jobs/apply_free_movement.h"
#include "jobs/entity_job_scheduler.h"
#include "resources/res_manager.h"
#include "dm/store.h"
#include "dm/store_trace.h"
#include "dm/streamer.h"
#include "dm/tracer_buffered.h"
#include "dm/tracer_sync.h"
#include "om/entity.h"
#include "om/stonehearth.h"
#include "om/components/clock.ridl.h"
#include "om/components/mod_list.ridl.h"
#include "om/components/mob.ridl.h"
#include "om/components/terrain.ridl.h"
#include "om/components/entity_container.ridl.h"
#include "om/components/data_store.ridl.h"
//#include "native_commands/create_room_cmd.h"
#include "jobs/job.h"
#include "lib/lua/script_host.h"
#include "lib/lua/caching_allocator.h"
#include "lib/lua/res/open.h"
#include "lib/lua/rpc/open.h"
#include "lib/lua/sim/open.h"
#include "lib/lua/om/open.h"
#include "lib/lua/dm/open.h"
#include "lib/lua/voxel/open.h"
#include "lib/lua/physics/open.h"
#include "lib/lua/analytics/open.h"
#include "om/lua/lua_om.h"
#include "om/error_browser/error_browser.h"
#include "csg/lua/lua_csg.h"
#include "lib/rpc/session.h"
#include "lib/rpc/function.h"
#include "lib/rpc/core_reactor.h"
#include "lib/rpc/reactor_deferred.h"
#include "lib/rpc/protobuf_reactor.h"
#include "lib/rpc/trace_object_router.h"
#include "lib/rpc/lua_module_router.h"
#include "lib/rpc/lua_object_router.h"
#include "core/profiler.h"
#include "remote_client.h"
#include "lib/perfmon/frame.h"
#include "lib/perfmon/namespace.h"
#include "lib/perfmon/sampling_profiler.h"
#include "platform/sysinfo.h"
#include "physics/water_tight_region_builder.h"


// Uncomment this to only profile the pathfinding path in VTune
// #define PROFILE_ONLY_PATHFINDING 1

using namespace ::radiant;
using namespace ::radiant::simulation;

namespace proto = ::radiant::tesseract::protocol;

static const int LUA_MEMORY_STATS_INTERVAL = 60 * 1000;

#define SIM_LOG(level)              LOG(simulation.core, level)
#define SIM_LOG_GAMELOOP(level)     LOG_CATEGORY(simulation.core, level, "simulation.core (time left: " << game_loop_timer_.remaining() << ")")

DEFINE_SINGLETON(Simulation);

Simulation::Simulation() :
   store_(nullptr),
   waiting_for_client_(true),
   noidle_(false),
   _tcp_acceptor(nullptr),
   _showDebugNodes(false),
   _singleStepPathFinding(false),
   begin_loading_(false),
   _sequenceNumber(1),
   _profileLongFrames(false)
{
   OneTimeIninitializtion();
}

void Simulation::OneTimeIninitializtion()
{
   // options...
   core::Config &config = core::Config::GetInstance();
   game_tick_interval_ = config.Get<int>("simulation.game_tick_interval", 50);
   net_send_interval_ = config.Get<int>("simulation.net_send_interval", 50);
   base_walk_speed_ = config.Get<float>("simulation.base_walk_speed", 7.5f);
   game_speed_ = config.Get<float>("simulation.game_speed", 1.0f);
   game_speed_ = config.Get<float>("simulation.game_speed", 1.0f);
   base_walk_speed_ = base_walk_speed_ * game_tick_interval_ / 1000.0f;

   _allocDataStoreFn = [this]() {
      return AllocDatastore();
   };

   // sessions (xxx: stub it out for single player)
   session_ = std::make_shared<rpc::Session>();
   session_->player_id = "player_1";

   // reactors...
   core_reactor_ = std::make_shared<rpc::CoreReactor>();
   protobuf_reactor_ = std::make_shared<rpc::ProtobufReactor>(*core_reactor_, [this](proto::PostCommandReply const& r) {
      SendReply(r);
   });
   enable_job_logging_ = config.Get<bool>("enable_job_logging", false);
   if (enable_job_logging_) {
      SIM_LOG(0) << "job logging enabled";
      jobs_perf_guard_ = perf_jobs_.OnFrameEnd([this](perfmon::Frame* frame) {
         LogJobPerfCounters(frame);
      });
   }

   // routes...
   core_reactor_->AddRouteV("radiant:debug_navgrid", [this](rpc::Function const& f) {
      json::Node args = f.args;
      debug_navgrid_mode_ = args.get<std::string>("mode", "none");
      if (debug_navgrid_mode_ != "none") {
         debug_navgrid_point_ = args.get<csg::Point3>("cursor", csg::Point3::zero);
         om::EntityPtr pawn = GetStore().FetchObject<om::Entity>(args.get<std::string>("pawn", ""));

         if (pawn != debug_navgrid_pawn_.lock()) {
            debug_navgrid_pawn_ = pawn;
            if (pawn) {
               SIM_LOG(0) << "setting debug nav grid pawn to " << *pawn;
            } else {
               SIM_LOG(0) << "clearing debug nav grid pawn";
            }
         }
      }
   });

   core_reactor_->AddRouteJ("radiant:query_pathfinder_info", [this](rpc::Function const& f) {
      json::Node args(f.args);
      json::Node pathfinders(JSON_ARRAY);

      csg::Point3 pt = args.get<csg::Point3>(0);
      ForEachPathFinder([this, &pt, &pathfinders](PathFinderPtr const& pf) mutable {
         if (pf->OpenSetContains(pt)) {
            json::Node entry;
            pf->GetPathFinderInfo(entry);
            pathfinders.add(entry);
         }
      });
      json::Node result;
      result.set("pathfinders", pathfinders);
      SIM_LOG(0) << "-- radiant:query_pathfinder_info result -------------------";
      SIM_LOG(0) << result.write_formatted();
      return result;
   });

   core_reactor_->AddRouteS("radiant:toggle_debug_nodes", [this](rpc::Function const& f) {
      _showDebugNodes = !_showDebugNodes;
      std::string msg = BUILD_STRING("debug nodes turned " << (_showDebugNodes ? "ON" : "OFF"));
      SIM_LOG(0) << msg;
      return msg;
   });

   core_reactor_->AddRouteS("radiant:toggle_step_paths", [this](rpc::Function const& f) {
      _singleStepPathFinding = !_singleStepPathFinding;
      std::string msg = BUILD_STRING("single step path finding turned " << (_singleStepPathFinding ? "ON" : "OFF"));
      SIM_LOG(0) << msg;
      return msg;
   });
   core_reactor_->AddRouteV("radiant:step_paths", [this](rpc::Function const& f) {
      StepPathFinding();
   });
   core_reactor_->AddRoute("radiant:game:start_task_manager", [this](rpc::Function const& f) {
      return StartTaskManager();
   });
   core_reactor_->AddRoute("radiant:game:get_perf_counters", [this](rpc::Function const& f) {
      return StartPerformanceCounterPush();
   });
   core_reactor_->AddRoute("radiant:game:set_game_speed", [this](rpc::Function const& f) {
      rpc::ReactorDeferredPtr result = std::make_shared<rpc::ReactorDeferred>("set game speed");
      try {
         json::Node node(f.args);
         game_speed_ = node.get<float>(0, 1.0f);
      } catch (std::exception const& e) {
         result->RejectWithMsg(BUILD_STRING("exception: " << e.what()));
      }
      return result;
   });
   core_reactor_->AddRoute("radiant:game:get_game_speed", [this](rpc::Function const& f) {
      rpc::ReactorDeferredPtr result = std::make_shared<rpc::ReactorDeferred>("get game speed");
      try {
         json::Node node;
         node.set("game_speed", game_speed_);
         result->Resolve(node);
      } catch (std::exception const& e) {
         result->RejectWithMsg(BUILD_STRING("exception: " << e.what()));
      }
      return result;
   });
   core_reactor_->AddRouteV("radiant:toggle_cpu_profile", [this](rpc::Function const& f) {
      bool profileLongFrame = core::Config::GetInstance().Get<bool>("simulation.profile_long_frames", true);
      if (profileLongFrame) {
         _profileLongFrames = !_profileLongFrames;
         SIM_LOG(0) << "lua long frame cpu profile is " << (_profileLongFrames ? "on" : "off");
         return;
      }

      bool enabled = !scriptHost_->IsCpuProfilerRunning();
      if (enabled) {
         scriptHost_->StopCpuProfiling(true);
      } else {
         scriptHost_->StartCpuProfiling(lua::ScriptHost::CpuProfilerMethod::Default, -1);
      }
      SIM_LOG(0) << "lua cpu profile is " << (enabled ? "on" : "off");
   });
   core_reactor_->AddRouteV("radiant:write_lua_memory_profile", [this](rpc::Function const& f) {
      scriptHost_->WriteMemoryProfile("lua_memory_profile.txt");      
      scriptHost_->DumpHeap("lua.heap");      
   });

   core_reactor_->AddRoute("radiant:server:get_error_browser", [this](rpc::Function const& f) {
      rpc::ReactorDeferredPtr d = std::make_shared<rpc::ReactorDeferred>("get error browser");
      json::Node obj;
      obj.set("error_browser", error_browser_->GetStoreAddress());
      d->Resolve(obj);
      return d;
   });

   core_reactor_->AddRouteV("radiant:server:save", [this](rpc::Function const& f) {
      Save(json::Node(f.args).get<std::string>("saveid"));
   });
   core_reactor_->AddRoute("radiant:server:load", [this](rpc::Function const& f) {
      return BeginLoad(json::Node(f.args).get<std::string>("saveid"));
   });
   core_reactor_->AddRoute("radiant:show_pathfinder_time", [this](rpc::Function const& f) {
      rpc::ReactorDeferredPtr d = std::make_shared<rpc::ReactorDeferred>("show pathfinder time");
      _bottomLoopFns.emplace_back([this, d]() {
         JSONNode obj;
         for (auto const& entry : entity_jobs_schedulers_) {
            EntityJobSchedulerPtr scheduler = entry.second;
            scheduler->SetRecordPathfinderTimes(true);

            om::EntityPtr entity = scheduler->GetEntity().lock();
            if (entity) {
               JSONNode subobj;
               perfmon::CounterValueType total = 0;

               EntityJobScheduler::PathfinderTimeMap const& times = scheduler->GetPathfinderTimes();
               for (auto const& entry : times) {
                  total += entry.second;
                  subobj.push_back(JSONNode(entry.first, perfmon::CounterToMilliseconds(entry.second)));
               }
               scheduler->ResetPathfinderTimes(); // xxx: this will screw up future bottom loop handlers =(

               subobj.push_back(JSONNode("total", total));
               subobj.set_name(BUILD_STRING(*entity));
               obj.push_back(subobj);
            }
         }
         d->Notify(obj);
      });
      return d;
   });
}


void Simulation::Initialize()
{
   InitializeDataObjects();
   InitializeGameObjects();
}

void Simulation::Shutdown()
{
   scriptHost_->Shutdown();
   store_->DisableAndClearTraces();
   ShutdownLuaObjects();
   ShutdownDataObjectTraces();
   ShutdownGameObjects();
   ShutdownDataObjects();
}

void Simulation::InitializeDataObjects()
{
   store_.reset(new dm::Store(1, "game"));
   om::RegisterObjectTypes(*store_);
}

void Simulation::ShutdownDataObjects()
{
   SIM_LOG(1) << "Shutting down data objects.";
   store_.reset();
}

void Simulation::InitializeDataObjectTraces()
{
   object_model_traces_ = std::make_shared<dm::TracerSync>("sim objects");
   pathfinder_traces_ = std::make_shared<dm::TracerBuffered>("sim lua", *store_);
   lua_traces_ = std::make_shared<dm::TracerBuffered>("sim lua", *store_);

   store_->AddTracer(lua_traces_, dm::LUA_ASYNC_TRACES);
   store_->AddTracer(object_model_traces_, dm::LUA_SYNC_TRACES);
   store_->AddTracer(object_model_traces_, dm::OBJECT_MODEL_TRACES);
   store_->AddTracer(pathfinder_traces_, dm::PATHFINDER_TRACES);

   store_trace_ = store_->TraceStore("sim")->OnAlloced([=](dm::ObjectPtr const& obj) {
      dm::ObjectId id = obj->GetObjectId();
      dm::ObjectType type = obj->GetObjectType();
      if (type == om::EntityObjectType) {
         entityMap_[id] = std::static_pointer_cast<om::Entity>(obj);
      } else if (type == om::MobObjectType) {
         CreateFreeMotionTrace(std::static_pointer_cast<om::Mob>(obj));
      }
   })->PushStoreState();   
}

om::DataStorePtr Simulation::AllocDatastore()
{
   auto datastore = GetStore().AllocObject<om::DataStore>();
   dm::ObjectId id = datastore->GetObjectId();
#if defined(ENABLE_OBJECT_COUNTER)
   datastoreMap_[id] = datastore;
#endif
   return datastore;
}

void Simulation::ShutdownDataObjectTraces()
{
   SIM_LOG(1) << "Shutting down traces.";
   object_model_traces_.reset();
   pathfinder_traces_.reset();
   lua_traces_.reset();
   store_trace_.reset();
   buffered_updates_.clear();
}

void Simulation::InitializeGameObjects()
{
   octtree_ = std::unique_ptr<phys::OctTree>(new phys::OctTree(dm::OBJECT_MODEL_TRACES));
   octtree_->EnableSensorTraces(true);
   freeMotion_ = std::unique_ptr<phys::FreeMotion>(new phys::FreeMotion(octtree_->GetNavGrid()));
   if (core::Config::GetInstance().Get<bool>("mods.stonehearth.enable_water", true)) {
      waterTightRegionBuilder_ = std::unique_ptr<phys::WaterTightRegionBuilder>(new phys::WaterTightRegionBuilder(octtree_->GetNavGrid()));
   }

   scriptHost_.reset(new lua::ScriptHost("server"));

   lua_State* L = scriptHost_->GetInterpreter();
   lua_State* callback_thread = scriptHost_->GetCallbackThread();
   store_->SetInterpreter(callback_thread); // xxx move to dm open or something

   csg::RegisterLuaTypes(L);
   lua::om::open(L);
   lua::dm::open(L);
   lua::sim::open(L, this);
   lua::phys::open(L, *octtree_);
   lua::res::open(L);
   lua::voxel::open(L);
   lua::rpc::open(L, core_reactor_);
   lua::om::register_json_to_lua_objects(L, *store_);
   lua::analytics::open(L);

   luaModuleRouter_ = std::make_shared<rpc::LuaModuleRouter>(scriptHost_.get(), "server");
   luaObjectRouter_ = std::make_shared<rpc::LuaObjectRouter>(scriptHost_.get(), *store_);
   traceObjectRouter_ = std::make_shared<rpc::TraceObjectRouter>(*store_);
   core_reactor_->AddRouter(luaModuleRouter_);
   core_reactor_->AddRouter(luaObjectRouter_);
   core_reactor_->AddRouter(traceObjectRouter_);
}

void Simulation::ShutdownGameObjects()
{
   SIM_LOG(1) << "Shutting down game objects.";

   // Remove the entityMap datastructure before destroying the entities; this is because entity
   // destruction will cause other entities to be accessed/destroyed, but we'd need a valid
   // map for that.  This ensures that the map will be empty when those entities are cleared,
   // and so those lookups will fail (but without invalidating the map).
   {
      std::unordered_map<dm::ObjectId, om::EntityPtr> keepAlive = entityMap_;
      entityMap_.clear();
   }

   // The act of destroying entities may run lua code which may mutate our state.  Therefore,
   // make sure we clear out our state *after* they're all dead.
   entity_jobs_schedulers_.clear();
   jobs_.clear();
   tasks_.clear();
   freeMotionTasks_.clear();

   SIM_LOG(1) << "All entities and datastores have been destroyed.";

   clock_ = nullptr;
   root_entity_ = nullptr;
   modList_ = nullptr;

   task_manager_deferred_ = nullptr;
   perf_counter_deferred_ = nullptr;
   on_frame_end_guard_.Clear();

   core_reactor_->RemoveRouter(luaModuleRouter_);
   core_reactor_->RemoveRouter(luaObjectRouter_);
   core_reactor_->RemoveRouter(traceObjectRouter_);
   luaModuleRouter_ = nullptr;
   luaObjectRouter_ = nullptr;
   traceObjectRouter_ = nullptr;

   scriptHost_->SetNotifyErrorCb(nullptr);
   error_browser_.reset();
   scriptHost_.reset();

   waterTightRegionBuilder_.reset();
   freeMotion_.reset();
   octtree_.reset();
}

void Simulation::ShutdownLuaObjects()
{
   radiant_ = luabind::object();
}


void Simulation::CreateGame()
{   
   root_entity_ = GetStore().AllocObject<om::Entity>();
   ASSERT(root_entity_->GetObjectId() == 1);
   clock_ = root_entity_->AddComponent<om::Clock>();
   now_ = clock_->GetTime();
   modList_ = root_entity_->AddComponent<om::ModList>();

   root_entity_->AddComponent<om::Clock>();

   om::TerrainPtr terrain = root_entity_->AddComponent<om::Terrain>();
   if (waterTightRegionBuilder_) {
      waterTightRegionBuilder_->SetWaterTightRegion(terrain->GetWaterTightRegion(),
                                                    terrain->GetWaterTightRegionDelta());
   }

   error_browser_ = store_->AllocObject<om::ErrorBrowser>();
   scriptHost_->SetNotifyErrorCb([=](om::ErrorBrowser::Record const& r) {
      error_browser_->AddRecord(r);
   });

   InitializeDataObjectTraces();
   GetOctTree().SetRootEntity(root_entity_);

   try {
      radiant_ = scriptHost_->Require("radiant.server");
      scriptHost_->CreateGame(modList_, _allocDataStoreFn);
   } catch (std::exception const& e) {
      scriptHost_->ReportCStackException(scriptHost_->GetInterpreter(), e);
   }
}

Simulation::~Simulation()
{
   // Control the order that items get destroyed for graceful shutdown.
   // xxx: this doesnt' work at all.  use std::shared_ptrs<> to keep things alive
   // (really only done by the simulation manager) and std::weak_ptrs<> to hold
   // references to things, with explicit lifetime registration callbacks
   // in the new Storage system if you REALLY care. (xxx)
   octtree_.release();
}

rpc::ReactorDeferredPtr Simulation::StartTaskManager()
{
   if (!task_manager_deferred_) {
      task_manager_deferred_ = std::make_shared<rpc::ReactorDeferred>("game task_manager");
      on_frame_end_guard_ = perf_timeline_.OnFrameEnd([this](perfmon::Frame* frame) {
         json::Node times(JSONNode(JSON_ARRAY));
         int total_time = 0;
         for (perfmon::Counter const* counter : frame->GetCounters()) {
            json::Node entry;
            perfmon::CounterValueType time = counter->GetValue();
            int ms = perfmon::CounterToMilliseconds(time);
            entry.set("name", counter->GetName());
            entry.set("time", ms);
            times.add(entry);
            total_time += ms;
         }
         json::Node summary;
         summary.set("counters", times);
         summary.set("total_time", total_time);
         task_manager_deferred_->Notify(summary);
      });
   }
   return task_manager_deferred_;
}

rpc::ReactorDeferredPtr Simulation::StartPerformanceCounterPush()
{
   if (!perf_counter_deferred_) {
      perf_counter_deferred_ = std::make_shared<rpc::ReactorDeferred>("game perf counters");
      next_counter_push_.set(0);
   }
   return perf_counter_deferred_ ;
}

void Simulation::PushPerformanceCounters()
{
   if (perf_counter_deferred_) {
      json::Node counters(JSON_ARRAY);

      auto addCounter = [&counters](const char* name, double value, const char* type) {
         json::Node row;
         row.set("name", name);
         row.set("value", value);
         row.set("type", type);
         counters.add(row);
      };

      AStarPathFinder::ComputeCounters(addCounter);
      GetScript().ComputeCounters(addCounter);

      perf_counter_deferred_->Notify(counters);
   }
}


void Simulation::StepPathFinding()
{
   platform::timer t(1000);
   radiant::stdutil::ForEachPrune<Job>(jobs_, [&](std::shared_ptr<Job> &p) {
      if (!p->IsFinished() && !p->IsIdle()) {
         p->Work(t);
      }
   });
}

void Simulation::FireLuaTraces()
{
   MEASURE_TASK_TIME(perf_timeline_, "lua")

   SIM_LOG_GAMELOOP(7) << "firing lua traces";
   lua_traces_->Flush();
}

void Simulation::UpdateGameState()
{
   MEASURE_TASK_TIME(perf_timeline_, "lua")

   // Run AI...
   SIM_LOG_GAMELOOP(7) << "calling lua update";
   try {
      radiant_["update"]();
   } catch (std::exception const& e) {
      SIM_LOG(3) << "fatal error initializing game update: " << e.what();
      GetScript().ReportCStackThreadException(GetScript().GetCallbackThread(), e);
   }
}

void Simulation::EncodeBeginUpdate(std::shared_ptr<RemoteClient> c)
{
   proto::Update update;
   update.set_type(proto::Update::BeginUpdate);
   auto msg = update.MutableExtension(proto::BeginUpdate::extension);
   msg->set_sequence_number(_sequenceNumber);
   c->SetSequenceNumber(_sequenceNumber);
   c->send_queue->Push(update);
}

void Simulation::EncodeEndUpdate(std::shared_ptr<RemoteClient> c)
{
   proto::Update update;

   update.set_type(proto::Update::EndUpdate);
   c->send_queue->Push(update);
}

void Simulation::EncodeServerTick(std::shared_ptr<RemoteClient> c)
{
   proto::Update update;

   int interval = static_cast<int>(game_speed_ * game_tick_interval_);

   update.set_type(proto::Update::SetServerTick);
   auto msg = update.MutableExtension(proto::SetServerTick::extension);
   msg->set_now(now_);
   msg->set_interval(interval);
   msg->set_next_msg_time(net_send_interval_);
   SIM_LOG_GAMELOOP(7) << "sending server tick " << now_;
   c->send_queue->Push(update);
}

void Simulation::EncodeUpdates(std::shared_ptr<RemoteClient> c)
{
   for (const auto& msg : buffered_updates_) {
      c->send_queue->Push(msg);
   }
   buffered_updates_.clear();
}

void Simulation::FlushStream(std::shared_ptr<RemoteClient> c)
{
   c->streamer->Flush();
}

om::EntityPtr Simulation::GetRootEntity()
{
   return root_entity_;
}

phys::OctTree &Simulation::GetOctTree()
{
   return *octtree_;
}

om::EntityPtr Simulation::GetEntity(dm::ObjectId id)
{
   auto i = entityMap_.find(id);
   return i != entityMap_.end() ? i->second : nullptr;
}

void Simulation::DestroyEntity(dm::ObjectId id)
{
   auto i = entityMap_.find(id);
   if (i != entityMap_.end()) {
      // remove from the map before triggering so lookups will fail
      om::EntityPtr entity = i->second;
      entityMap_.erase(i);

      lua_State* L = scriptHost_->GetInterpreter();
      luabind::object e(L, std::weak_ptr<om::Entity>(entity));
      luabind::object id(L, entity->GetObjectId());
      luabind::object evt(L, luabind::newtable(L));
      evt["entity"] = e;
      evt["entity_id"] = id;

      scriptHost_->TriggerOn(e, "radiant:entity:pre_destroy", evt);
      scriptHost_->Trigger("radiant:entity:pre_destroy", evt);

      evt = luabind::object(L, luabind::newtable(L));
      evt["entity_id"] = id;

      entity->Destroy();
      entity = nullptr;

      scriptHost_->Trigger("radiant:entity:post_destroy", evt);
   }
}


dm::Store& Simulation::GetStore()
{
   return *store_;
}


void Simulation::AddTask(std::shared_ptr<Task> task)
{
   tasks_.push_back(task);
}

void Simulation::AddJob(std::shared_ptr<Job> job)
{
   SIM_LOG_GAMELOOP(5) << "adding job jid:" << job->GetId() << " name:" << job->GetName();
   jobs_.push_back(job);
}

void Simulation::RemoveJobForEntity(om::EntityPtr entity, PathFinderPtr pf)
{
   if (entity) {
      dm::ObjectId id = entity->GetObjectId();
      auto i = entity_jobs_schedulers_.find(id);
      if (i != entity_jobs_schedulers_.end()) {
         i->second->RemovePathfinder(pf);
      }
   }
}

void Simulation::AddJobForEntity(om::EntityPtr entity, PathFinderPtr pf)
{
   if (entity) {
      dm::ObjectId id = entity->GetObjectId();
      auto i = entity_jobs_schedulers_.find(id);
      if (i == entity_jobs_schedulers_.end()) {
         EntityJobSchedulerPtr ejs = std::make_shared<EntityJobScheduler>(entity);
         i = entity_jobs_schedulers_.insert(make_pair(id, ejs)).first;
         jobs_.push_back(ejs);
         SIM_LOG_GAMELOOP(5) << "created entity job scheduler " << ejs->GetId() << " for " << (*entity);
      }
      SIM_LOG_GAMELOOP(5) << "adding job jid:" << pf->GetId() << " for " << (*entity) << " scheduler:" << i->second->GetId();
      i->second->AddPathfinder(pf);
   }
}

void Simulation::EncodeDebugShapes(protocol::SendQueuePtr queue)
{
   proto::Update update;
   update.set_type(proto::Update::UpdateDebugShapes);
   auto uds = update.MutableExtension(proto::UpdateDebugShapes::extension);
   auto msg = uds->mutable_shapelist();

   if (_showDebugNodes) {
      radiant::stdutil::ForEachPrune<Job>(jobs_, [&](std::shared_ptr<Job> &job) {
         job->EncodeDebugShapes(msg);
      });
   }
   if (debug_navgrid_mode_ == "navgrid") {
      GetOctTree().GetNavGrid().ShowDebugShapes(debug_navgrid_point_, debug_navgrid_pawn_, msg);
   } else if (debug_navgrid_mode_ == "water_tight") {
      if (waterTightRegionBuilder_) {
         waterTightRegionBuilder_->ShowDebugShapes(debug_navgrid_point_, msg);
      }
   }
   for (auto const& cb : _bottomLoopFns)  {
      cb();
   }
   queue->Push(update);
}

void Simulation::ProcessTaskList()
{
   MEASURE_TASK_TIME(perf_timeline_, "native tasks")
   SIM_LOG_GAMELOOP(7) << "processing task list";

   freeMotion_->ProcessDirtyTiles(game_loop_timer_);

   auto i = tasks_.begin();
   while (i != tasks_.end()) {
      std::shared_ptr<Task> task = i->lock();

      if (task && task->Work(game_loop_timer_)) {
         ++i;
      } else {
         i = tasks_.erase(i);
      }
   }
}

void Simulation::ProcessJobList()
{
   MEASURE_TASK_TIME(perf_timeline_, "pathfinder")

   // Pahtfinders down here...
   if (_singleStepPathFinding) {
      SIM_LOG(5) << "skipping job processing (single step is on).";
      return;
   }
   pathfinder_traces_->Flush();

   SIM_LOG_GAMELOOP(7) << "processing job list";

#if defined(PROFILE_ONLY_PATHFINDING)
   core::SetProfilerEnabled(true);
#endif

   int idleCountdown = (int)jobs_.size();
   while (!game_loop_timer_.expired() && !jobs_.empty() && idleCountdown) {
      std::weak_ptr<Job> front = radiant::stdutil::pop_front(jobs_);
      std::shared_ptr<Job> job = front.lock();
      if (job) {
         SIM_LOG_GAMELOOP(9) << "checking job jid:" << job->GetId();
         if (!job->IsFinished()) {
            if (!job->IsIdle()) {
               idleCountdown = (int)jobs_.size() + 2;
               job->Work(game_loop_timer_);
            }
            jobs_.push_back(front);
         } else {
            SIM_LOG_GAMELOOP(5) << "jid:" << job->GetId() << " is finished.  removing from job list.";
         }
      } else {
         SIM_LOG_GAMELOOP(7) << "job failed to lock.  removing from job list";
      }
      idleCountdown--;
   }

#if defined(PROFILE_ONLY_PATHFINDING)
   core::SetProfilerEnabled(false);
#endif

   SIM_LOG_GAMELOOP(7) << "done processing job list";
}

void Simulation::PostCommandRequest(proto::PostCommandRequest const& request)
{
   protobuf_reactor_->Dispatch(session_, request);
}

void Simulation::FinishedUpdate(proto::FinishedUpdate const& msg)
{
   waiting_for_client_ = false;
   for (std::shared_ptr<RemoteClient> c : _clients) {
      // xxx: when we do multiplayer, this needs to be keyed by session
      c->AckSequenceNumber(msg.sequence_number());
   }
}

bool Simulation::ProcessMessage(std::shared_ptr<RemoteClient> c, const proto::Request& msg)
{
#define DISPATCH_MSG(MsgName) \
   case proto::Request::MsgName: \
      MsgName(msg.GetExtension(proto::MsgName::extension)); \
      break;

   switch (msg.type()) {
      DISPATCH_MSG(PostCommandRequest)
      DISPATCH_MSG(FinishedUpdate)
   default:
      ASSERT(false);
   }
   return true;
}

void Simulation::SendReply(proto::PostCommandReply const& reply)
{
   proto::Update msg;
   msg.set_type(proto::Update::PostCommandReply);
   proto::PostCommandReply* r = msg.MutableExtension(proto::PostCommandReply::extension);
   *r = reply; // is this kosher?

   SIM_LOG(5) << "adding buffered reply.............";
   buffered_updates_.emplace_back(msg);
}

lua::ScriptHost& Simulation::GetScript() {
   return *scriptHost_;
}

void Simulation::Run(tcp::acceptor* acceptor, boost::asio::io_service* io_service)
{
   _tcp_acceptor = acceptor;
   _io_service = io_service;
   noidle_ = core::Config::GetInstance().Get("game.noidle", false);
   main();
}

void Simulation::handle_accept(std::shared_ptr<tcp::socket> socket, const boost::system::error_code& error)
{
   if (!error) {
      std::shared_ptr<RemoteClient> c = std::make_shared<RemoteClient>();

      c->socket = socket;
      c->send_queue = protocol::SendQueue::Create(*socket, "server");
      c->recv_queue = std::make_shared<protocol::RecvQueue>(*socket, "server");
      c->streamer = std::make_shared<dm::Streamer>(*store_, dm::PLAYER_1_TRACES, c->send_queue.get());
      _clients.push_back(c);

      //queue_network_read(c, protocol::alloc_buffer(1024 * 1024));
   }
   start_accept();
}

void Simulation::start_accept()
{
   std::shared_ptr<tcp::socket> socket(new tcp::socket(*_io_service));
   auto cb = bind(&Simulation::handle_accept, this, socket, std::placeholders::_1);
   _tcp_acceptor->async_accept(*socket, cb);
}

void Simulation::main()
{
   _tcp_acceptor->listen();
   start_accept();

   Initialize();
   CreateGame();

   unsigned int last_stat_dump = 0;

   net_send_timer_.set(0); // the first update gets pushed out immediately
   game_loop_timer_.set(game_tick_interval_);
   while (1) {
      Mainloop();
      ++_sequenceNumber;
   }
}

void Simulation::Mainloop()
{
   SIM_LOG_GAMELOOP(7) << "starting next gameloop";

   if (next_counter_push_.expired()) {
      perf_timeline_.BeginFrame();
      PushPerformanceCounters();
      next_counter_push_.set(500);
   }
   if (lua_memory_timer_.expired()) {
      lua::CachingAllocator::GetInstance().ReportMemoryStats();
      lua_memory_timer_.set(LUA_MEMORY_STATS_INTERVAL);
   }
   if (enable_job_logging_ && log_jobs_timer_.expired()) {
      perf_jobs_.BeginFrame();
      log_jobs_timer_.set(2000);
   }
   if (_profileLongFrames) {
      scriptHost_->StartCpuProfiling(lua::ScriptHost::CpuProfilerMethod::TimeAccumulation, -1);
   }

   ReadClientMessages();

   if (begin_loading_) {
      waiting_for_client_ = true;
      begin_loading_ = false;
      Load();
   }

   if (!waiting_for_client_) {
      int interval = static_cast<int>(game_speed_ * game_tick_interval_);
      now_ = now_ + interval;
      clock_->SetTime(now_);

      scriptHost_->Trigger("radiant:gameloop:start");

      UpdateGameState();
      if (_profileLongFrames) {
         bool tooLong = game_loop_timer_.expired();
         if (tooLong) {
            SIM_LOG(0) << "frame too long by " << game_loop_timer_.remaining() << ".  dumping profile!";
         }
         scriptHost_->StopCpuProfiling(tooLong);
      }
      
      ProcessTaskList();
      ProcessJobList();
      FireLuaTraces();
      octtree_->GetNavGrid().UpdateGameTime(now_, game_tick_interval_);
      if (waterTightRegionBuilder_) {
         waterTightRegionBuilder_->UpdateRegion();
      }

      scriptHost_->Trigger("radiant:gameloop:end");
   }

   // Disable the independant netsend timer until I can figure out this clock synchronization stuff -- tonyc
   static const bool disable_net_send_timer = true;
   if (disable_net_send_timer || net_send_timer_.expired()) {
      SendClientUpdates(true);
      // reset the send timer.  We have a choice here of using "set" or "advance". Set
      // will set the timer to the current time + the interval.  Advance will set it to
      // the exact time we were *supposed* to render + the interval.  When the server is
      // flush with CPU and se spend some time idling, these two numbers are nearly
      // identical.  For very long ticks or when the server is just plain overloaded, however,
      // "set" throttle the send rate, guaranteeing we get at least net_send_interval_ ms
      // of simulation time before the update is sent.  
      net_send_timer_.set(net_send_interval_); 
   } else {
      SIM_LOG_GAMELOOP(7) << "net log still has " << net_send_timer_.remaining() << " till expired.";
   }
   LuaGC();
   Idle();
}

void Simulation::LuaGC()
{
   MEASURE_TASK_TIME(perf_timeline_, "lua gc")
   SIM_LOG_GAMELOOP(7) << "starting lua gc";
   scriptHost_->GC(game_loop_timer_);
}

void Simulation::Idle()
{
#if defined(ENABLE_OBJECT_COUNTER)
   static int nextAuditTime = 0;

   int now = platform::get_current_time_in_ms();
   if (nextAuditTime == 0 || nextAuditTime < now) {
      std::unordered_map<std::string, int> counts;
      for (auto const& entry : core::ObjectCounterBase::GetObjects()) {
         std::type_index const& objType = entry.first;
         core::ObjectCounterBase::AllocationMap const& am = entry.second;

         if (objType == typeid(om::DataStore)) {
            for (auto const& alloc : am.allocs) {
               om::DataStore const* ds = static_cast<om::DataStore const*>(alloc.first);
               std::string const& controllerName = ds->GetControllerName();
               if (!controllerName.empty()) {
                  counts[controllerName + std::string(" controller")] += 1;
               } else {
                  counts[std::string(objType.name())] += 1;
               }
            }
         } else {
            counts[std::string(objType.name())] += am.allocs.size();
         }
      };
      typedef std::pair<std::string, int> NameCount;
      std::vector<NameCount> sortedCounts;
      for (auto const& entry : counts) {
         sortedCounts.push_back(entry);
      }
      std::sort(sortedCounts.begin(), sortedCounts.end(), [](NameCount const& lhs, NameCount const& rhs) {
         return lhs.second > rhs.second;
      });
      SIM_LOG(0) << "== Object Count Audit at " << now << " ============================";
      for (auto const& entry : sortedCounts) {
         SIM_LOG(0) << "     " << std::setw(10) << entry.second << " " << entry.first;
      }

      SIM_LOG(0) << "-- Traces -----------------------------------";
      int total = 0;
      for (auto const& entry : store_->GetTraceReasons()) {
         const char* reason = entry.first;
         int count = entry.second;
         SIM_LOG(0) << "     " << std::setw(10) << count << " : "  << reason;
         total += count;
      }
      SIM_LOG(0) << "     " << std::setw(10) << total << " : "  << "TOTAL";

      SIM_LOG(0) << "-- Datastores -----------------------------------";
      {
         std::unordered_map<std::string, std::unordered_map<int, int>> counts;
         auto i = datastoreMap_.begin(), end = datastoreMap_.end();
         while (i != end) {
            int c = i->second.use_count();
            if (c > 0) {
               std::string controllerName = i->second.lock()->GetControllerName();
               if (controllerName.empty()) {
                  controllerName = "unnamed";
               }
               ++counts[controllerName][c];
               ++i;
            } else {
               i = datastoreMap_.erase(i);
            }
         };

         for (auto const& entry : counts) {
            std::string const& name = entry.first;
            for (auto const& e : entry.second) {
               int useCount = e.first;
               int total = e.second;
               SIM_LOG(1) << "     " << std::setw(40) << name << " useCount:" << useCount << "   total:" << total;
            }
         }
      }

      nextAuditTime = now + 5000;
   }
#endif

   if (!noidle_) {
      MEASURE_TASK_TIME(perf_timeline_, "idle")
      SIM_LOG_GAMELOOP(7) << "idling";
      while (!game_loop_timer_.expired()) {
         platform::sleep(1);
      }
   } else {
      if (!game_loop_timer_.expired()) {
         perfmon::Counter* last_counter = perf_timeline_.GetCurrentCounter();
         perfmon::Counter* counter = perf_timeline_.GetCounter("idle");
         perf_timeline_.SetCounter(counter);
         counter->Increment(perfmon::MillisecondsToCounter(game_loop_timer_.remaining()));
         perf_timeline_.SetCounter(last_counter);
         // Idle a little bit, or the client can't keep up!
         Sleep(2);
      }
   }
   game_loop_timer_.set(game_tick_interval_);
}

void Simulation::SendClientUpdates(bool throttle)
{
   MEASURE_TASK_TIME(perf_timeline_, "network send")
   SIM_LOG_GAMELOOP(7) << "sending client updates (time left on net timer: " << net_send_timer_.remaining() << ")";

   for (std::shared_ptr<RemoteClient> c : _clients) {
      SendUpdates(c, throttle);
   };
}

void Simulation::SendUpdates(std::shared_ptr<RemoteClient> c, bool throttle)
{   
   if (throttle && !c->IsClientReadyForUpdates()) {
      return;
   }
   if (c->streamer) {
      EncodeBeginUpdate(c);
      EncodeServerTick(c);
      FlushStream(c);
      EncodeDebugShapes(c->send_queue);
      EncodeEndUpdate(c);
   }
   EncodeUpdates(c);
   c->FlushSendQueue();
}

void Simulation::ReadClientMessages()
{
   MEASURE_TASK_TIME(perf_timeline_, "network recv")
   SIM_LOG_GAMELOOP(7) << "processing messages";

   _io_service->poll();
   _io_service->reset();

   // We'll let the simulation process as many messages as it wants.
   platform::timer timeout(100000);
   for (auto c : _clients) {
      c->recv_queue->Process<proto::Request>([=](proto::Request const& msg) -> bool {
         return ProcessMessage(c, msg);
      }, timeout);
   }
}

float Simulation::GetBaseWalkSpeed() const
{
   return base_walk_speed_ * game_speed_;
}

void Simulation::Save(boost::filesystem::path const& saveid)
{
   ASSERT(store_);

   SIM_LOG(0) << "Starting save.";
   scriptHost_->FullGC();

   std::string error;
   std::string filename = (core::Config::GetInstance().GetSaveDirectory() / saveid / "server_state.bin").string();
   if (!store_->Save(filename, error)) {
      SIM_LOG(0) << "Failed to save: " << error;
   }
   SIM_LOG(0) << "Saved.";
}

rpc::ReactorDeferredPtr Simulation::BeginLoad(boost::filesystem::path const& saveid) {
   load_saveid_ = saveid;
   begin_loading_ = true;
   load_progress_deferred_ = std::make_shared<rpc::ReactorDeferred>("load progress");
   return load_progress_deferred_;
}

void Simulation::Load()
{
   SIM_LOG(0) << "Starting load.";

   // delete all the streamers before shutting down so we don't spam them with delete requests,
   // then create new streamers. 
   for (std::shared_ptr<RemoteClient> client : _clients) {
      client->streamer.reset();
   }

   // shutdown and initialize.  we need to initialize before creating the new streamers.  otherwise,
   // we won't have the new store for them.
   Shutdown();
   Initialize();

   std::string error;
   dm::ObjectMap objects;
   // Re-initialize the game
   std::string filename = (core::Config::GetInstance().GetSaveDirectory() / load_saveid_ / "server_state.bin").string();

#if defined(ENABLE_OBJECT_COUNTER)
   store_->TraceStore("sim")->OnAlloced([this](dm::ObjectPtr const& obj) mutable {
      if (obj->GetObjectType() == om::DataStoreObjectType) {
         dm::ObjectId id = obj->GetObjectId();
         om::DataStorePtr ds = std::static_pointer_cast<om::DataStore>(obj);
         datastoreMap_[ds->GetObjectId()] = ds;
      }
   })->PushStoreState();
#endif

   bool success = store_->Load(filename, error, objects, [this](int progress) {
      json::Node result;
      result.set("progress", progress);
      load_progress_deferred_->Notify(result);

      // Push the load progress notifications immediately.
      SendClientUpdates(false);
   });

   // Resolve or Reject the deferred depending on whether or not the load succeeded.  Then
   // send the updates to the client IMMEDIATELY.  This puts it in the send queue ahead of
   // all the data we just loaded.
   if (success) {
      load_progress_deferred_->Resolve(json::Node().set("progress", 100));
   } else {
      SIM_LOG(0) << "Failed to load: " << error;
      load_progress_deferred_->RejectWithMsg(error);
   }
   load_progress_deferred_ = nullptr;
   SendClientUpdates(false);
   
   // Finally, either finish loading the game or create a new one.
   if (success) {
      // Now put the streamers back so we can send the game state.
      for (std::shared_ptr<RemoteClient> client : _clients) {
         client->streamer = std::make_shared<dm::Streamer>(*store_, dm::PLAYER_1_TRACES, client->send_queue.get());
      }
      FinishLoadingGame();
   } else {
      // Trash everything and re-initialize.
      objects.clear();
      Shutdown();
      Initialize();

      // Create streamers AFTER we've cleared out our state, so all the crap that got loaded
      // and subsequently trashed won't be send down to the client.
      for (std::shared_ptr<RemoteClient> client : _clients) {
         client->streamer = std::make_shared<dm::Streamer>(*store_, dm::PLAYER_1_TRACES, client->send_queue.get());
      }

      // Now we can create a fresh new state and push updates to the clients.
      CreateGame();
      SendClientUpdates(false);
   }

   platform::SysInfo::LogMemoryStatistics("Finished Loading Simulation", 0);
}

#if defined(ENABLE_OBJECT_COUNTER)
void Simulation::AuditDatastores(const char* reason)
{
   std::unordered_map<std::string, std::unordered_map<int, int>> counts;
   for (auto const& entry : datastoreMap_) {
      int c = entry.second.use_count();
      if (c > 0) {
         std::string controllerName = entry.second.lock()->GetControllerName();
         if (controllerName.empty()) {
            controllerName = "unnamed";
         }
         ++counts[controllerName][c];
         if (controllerName == "radiant:controllers:timer") {
            // DebugBreak();
         }
      }
   };
   SIM_LOG(1) << "== Datastore Use Count Audit (" << reason << ") ============================";
   for (auto const& entry : counts) {
      std::string const& name = entry.first;
      for (auto const& e : entry.second) {
         int useCount = e.first;
         int total = e.second;
         SIM_LOG(1) << "     " << std::setw(40) << name << " useCount:" << useCount << "   total:" << total;
      }
   }
}
#endif

void Simulation::FinishLoadingGame()
{
   radiant_ = scriptHost_->Require("radiant.server");

   // Update the clock before sending client updates to make sure the first update
   // we send has the correct time.
   root_entity_ = store_->FetchObject<om::Entity>(1);
   ASSERT(root_entity_);
   clock_ = root_entity_->AddComponent<om::Clock>();
   now_ = clock_->GetTime();

   // Re-call SendClientUpdates().  This will send the data
   // for everything we just loaded to the client so it can get started re-creating its
   // state.  Notice that we do this *before* calling FinishLoadingGame so we can parallelize
   // the expensive task of restoring all the simulation metadata (navgrid, remotion traces,
   // etc.) with the client recreating all it's render state.
   SendClientUpdates(false);

   modList_ = root_entity_->AddComponent<om::ModList>();

   InitializeDataObjectTraces();
   store_->OnLoaded();
   GetOctTree().SetRootEntity(root_entity_);

   // do this after loading the datastores, so that the terrain component has a chance to perform any loading logic it needs
   om::TerrainPtr terrain = root_entity_->GetComponent<om::Terrain>();
   if (waterTightRegionBuilder_) {
      waterTightRegionBuilder_->SetWaterTightRegion(terrain->GetWaterTightRegion(),
                                                    terrain->GetWaterTightRegionDelta());
   }

   error_browser_ = store_->AllocObject<om::ErrorBrowser>();
   scriptHost_->SetNotifyErrorCb([=](om::ErrorBrowser::Record const& r) {
      error_browser_->AddRecord(r);
   });

   std::vector<om::DataStorePtr> datastores;
   store_->TraceStore("sim")->OnAlloced([&datastores, this](dm::ObjectPtr const& obj) mutable {
      if (obj->GetObjectType() == om::DataStoreObjectType) {

         dm::ObjectId id = obj->GetObjectId();
         om::DataStorePtr ds = std::static_pointer_cast<om::DataStore>(obj);
         datastores.emplace_back(ds);
#if defined(ENABLE_OBJECT_COUNTER)
         datastoreMap_[ds->GetObjectId()] = ds;
#endif
      }
   })->PushStoreState();

   scriptHost_->LoadGame(modList_, _allocDataStoreFn, entityMap_, datastores);
   datastores.clear();
}

void Simulation::Reset()
{
}

int Simulation::GetGameTickInterval() const
{
   return game_tick_interval_;
}

void Simulation::CreateFreeMotionTrace(om::MobPtr mob)
{
   dm::ObjectId id = mob->GetObjectId();
   om::MobRef m = mob;

   // If the mob ever goes into free-motion, create a task to move it around.
   // When it leaves free-motion, destroy the task.
   freeMotionTasks_[id].trace = mob->TraceInFreeMotion("free motion task", dm::OBJECT_MODEL_TRACES)
      ->OnChanged([this, id, m](bool inFreeMotion) {
         FreeMotionTaskMapEntry& entry = freeMotionTasks_[id];
         if (inFreeMotion) {
            if (!entry.task) {
               om::MobPtr mob = m.lock();
               if (mob) {
                  LOG(simulation.free_motion, 7) << "creating free motion task for entity " << id << " (no tasks exists yet)";
                  entry.task = std::make_shared<ApplyFreeMotionTask>(mob);
                  tasks_.push_back(entry.task);
               }
            }
         } else {
            LOG(simulation.free_motion, 7) << "destroying free motion task for entity " << id << " (left free motion state)";
            entry.task.reset();
         }
      })
      ->OnDestroyed([this, id]() {
         LOG(simulation.free_motion, 7) << "destroying free motion task for entity " << id << " (mob destroyed)";
         freeMotionTasks_.erase(id);
      })
      ->PushObjectState();
}

perfmon::Timeline& Simulation::GetOverviewPerfTimeline()
{
   return perf_timeline_;
}

perfmon::Timeline& Simulation::GetJobsPerfTimeline()
{
   return perf_jobs_;
}

void Simulation::LogJobPerfCounters(perfmon::Frame* frame)
{
   json::Node times(JSONNode(JSON_ARRAY));
   int totalTime = 0;

   SIM_LOG(1) << "--- job performance counters --------------------";
   std::set<std::pair<int, const char*>, std::greater<std::pair<int, const char*>>> counters;
   for (perfmon::Counter const* counter : frame->GetCounters()) {
      perfmon::CounterValueType time = counter->GetValue();
      int ms = perfmon::CounterToMilliseconds(time);
      const char* name = counter->GetName();
      counters.insert(std::make_pair(ms, name));
      totalTime += ms;
   }
   for (auto const& entry : counters) {
      bool found = false;
      int ms = entry.first;
      int percent = ms * 100 / totalTime;
      const char* name = entry.second;

      SIM_LOG(1) << std::setw(3) << percent << "% (" << std::setw(4) << ms << " ms) : " << GetProgressForJob(name);
   }
}

bool Simulation::GetEnableJobLogging() const
{
   return enable_job_logging_;
}

std::string Simulation::GetProgressForJob(core::StaticString name) const
{
   for (std::weak_ptr<Job> const& j : jobs_) {
      std::shared_ptr<Job> job = j.lock();
      if (job) {
         EntityJobSchedulerPtr ejs = std::dynamic_pointer_cast<EntityJobScheduler>(job);
         if (ejs) {
            for (auto const& entry : ejs->GetPathFinders()) {
               PathFinderPtr pf = entry.second.lock();
               if (pf && core::StaticString(pf->GetName()) == name) {
                  return pf->GetProgress();
               }
            }
         } else {
            if (core::StaticString(job->GetName()) == name) {
               return job->GetProgress();
            }
         }
      }
   }
   return BUILD_STRING(name << " (finished...)");
}

void Simulation::ForEachPathFinder(std::function<void(PathFinderPtr const&)> cb)
{
   for (std::weak_ptr<Job> const& j : jobs_) {
      std::shared_ptr<Job> job = j.lock();
      if (job) {
         EntityJobSchedulerPtr ejs = std::dynamic_pointer_cast<EntityJobScheduler>(job);
         if (ejs) {
            for (auto const& entry : ejs->GetPathFinders()) {
               PathFinderPtr pf = entry.second.lock();
               cb(pf);
            }
         }
      }
   }
}

