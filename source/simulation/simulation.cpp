#include "pch.h"
#include <sstream>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include "core/config.h"
#include "resources/manifest.h"
#include "radiant_exceptions.h"
#include "platform/utils.h"
#include "simulation.h"
#include "jobs/path_finder.h"
#include "jobs/follow_path.h"
#include "jobs/goto_location.h"
#include "jobs/entity_job_scheduler.h"
#include "resources/res_manager.h"
#include "dm/store.h"
#include "dm/store_trace.h"
#include "dm/streamer.h"
#include "dm/tracer_buffered.h"
#include "dm/tracer_sync.h"
#include "om/entity.h"
#include "om/components/clock.ridl.h"
#include "om/components/mob.ridl.h"
#include "om/components/terrain.ridl.h"
#include "om/components/entity_container.ridl.h"
#include "om/components/target_tables.ridl.h"
#include "om/object_formatter/object_formatter.h"
#include "om/components/data_store.ridl.h"
//#include "native_commands/create_room_cmd.h"
#include "jobs/job.h"
#include "lib/lua/script_host.h"
#include "lib/lua/res/open.h"
#include "lib/lua/rpc/open.h"
#include "lib/lua/sim/open.h"
#include "lib/lua/om/open.h"
#include "lib/lua/dm/open.h"
#include "lib/lua/voxel/open.h"
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
#include "remote_client.h"
#include "lib/perfmon/frame.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

namespace proto = ::radiant::tesseract::protocol;
namespace po = boost::program_options;

#define SIM_LOG(level)              LOG(simulation.core, level)
#define SIM_LOG_GAMELOOP(level)     LOG_CATEGORY(simulation.core, level, "simulation.core (time left: " << game_loop_timer_.remaining() << ")")

#define MEASURE_TASK_TIME(category)  perfmon::TimelineCounterGuard taskman_timer_ ## __LINE__ (perf_timeline_, category);

Simulation::Simulation() :
   _showDebugNodes(false),
   _singleStepPathFinding(false),
   store_(1, "game"),
   sequence_number_(1),
   paused_(false),
   noidle_(false),
   _tcp_acceptor(nullptr),
   debug_navgrid_enabled_(false),
   profile_next_lua_update_(false)
{
   octtree_ = std::unique_ptr<phys::OctTree>(new phys::OctTree(dm::OBJECT_MODEL_TRACES));
   scriptHost_.reset(new lua::ScriptHost());

   InitDataModel();

   // Entity 1
   root_entity_ = CreateEntity();
   ASSERT(root_entity_->GetObjectId() == 1);
   clock_ = GetEntity(1)->AddComponent<om::Clock>();

   error_browser_ = store_.AllocObject<om::ErrorBrowser>();
   scriptHost_->SetNotifyErrorCb([=](om::ErrorBrowser::Record const& r) {
      error_browser_->AddRecord(r);
   });

   // sessions (xxx: stub it out for single player)
   session_ = std::make_shared<rpc::Session>();
   session_->faction = "civ";

   // reactors...
   core_reactor_ = std::make_shared<rpc::CoreReactor>();
   protobuf_reactor_ = std::make_shared<rpc::ProtobufReactor>(*core_reactor_, [this](proto::PostCommandReply const& r) {
      SendReply(r);
   });

   // routers...
   core_reactor_->AddRouter(std::make_shared<rpc::LuaModuleRouter>(scriptHost_.get(), "server"));
   core_reactor_->AddRouter(std::make_shared<rpc::LuaObjectRouter>(scriptHost_.get(), store_));
   trace_router_ = std::make_shared<rpc::TraceObjectRouter>(store_);
   core_reactor_->AddRouter(trace_router_);

   core_reactor_->AddRouteV("radiant:debug_navgrid", [this](rpc::Function const& f) {
      json::Node args = f.args;
      debug_navgrid_enabled_ = args.get<bool>("enabled", false);
      if (debug_navgrid_enabled_) {
         debug_navgrid_point_ = args.get<csg::Point3>("cursor", csg::Point3::zero);
      }
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
   core_reactor_->AddRouteV("radiant:profile_next_lua_upate", [this](rpc::Function const& f) {
      profile_next_lua_update_ = true;
      SIM_LOG(0) << "profiling next lua update";
   });

   core_reactor_->AddRoute("radiant:server:get_error_browser", [this](rpc::Function const& f) {
      rpc::ReactorDeferredPtr d = std::make_shared<rpc::ReactorDeferred>("get error browser");
      json::Node obj;
      obj.set("error_browser", om::ObjectFormatter().GetPathToObject(error_browser_));
      d->Resolve(obj);
      return d;
   });

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


static std::string SanatizePath(std::string const& path)
{
   std::vector<std::string> s;
   boost::split(s, path, boost::is_any_of("/"));
   s.erase(std::remove(s.begin(), s.end(), ""), s.end());
   return std::string("/") + boost::algorithm::join(s, "/");
}

void Simulation::CreateNew()
{
   using namespace luabind;
   res::ResourceManager2 &resource_manager = res::ResourceManager2::GetInstance();

   lua_State* L = scriptHost_->GetInterpreter();
   lua_State* callback_thread = scriptHost_->GetCallbackThread();
   
   store_.SetInterpreter(callback_thread); // xxx move to dm open or something

   csg::RegisterLuaTypes(L);
   lua::om::open(L);
   lua::dm::open(L);
   lua::sim::open(L, this);
   lua::res::open(L);
   lua::voxel::open(L);
   lua::rpc::open(L, core_reactor_);
   lua::om::register_json_to_lua_objects(L, store_);
   lua::analytics::open(L);
   om::RegisterObjectTypes(store_);

   core::Config &config = core::Config::GetInstance();
   game_tick_interval_ = config.Get<int>("simulation.game_tick_interval", 50);
   net_send_interval_ = config.Get<int>("simulation.net_send_interval", 50);
   base_walk_speed_ = config.Get<float>("simulation.base_walk_speed", 7.5f);
   game_speed_ = config.Get<float>("simulation.game_speed", 1.0f);
   base_walk_speed_ = base_walk_speed_ * game_tick_interval_ / 1000.0f;

   game_api_ = scriptHost_->Require("radiant.server");
   for (std::string const& mod_name : resource_manager.GetModuleNames()) {
      json::Node manifest = resource_manager.LookupManifest(mod_name);
      std::string script_name = manifest.get<std::string>("server_init_script", "");
      if (!script_name.empty()) {
         try {
            luabind::globals(L)[mod_name] = scriptHost_->Require(script_name);
         } catch (std::exception const& e) {
            SIM_LOG(1) << "module " << mod_name << " failed to load " << script_name << ": " << e.what();
         }
      }
   }
   scriptHost_->Require("radiant.lualibs.strict");
   scriptHost_->Trigger("radiant:modules_loaded");

   std::string const module = config.Get<std::string>("game.mod");
   json::Node const manifest = resource_manager.LookupManifest(module);
   std::string const default_script = module + "/" + manifest.get<std::string>("game.script");

   std::string const game_script = config.Get("game.script", default_script);
   object game_ctor = scriptHost_->RequireScript(game_script);
   try {
      game_ = luabind::call_function<luabind::object>(game_ctor);
   } catch (std::exception const& e) {
      SIM_LOG(0) << "game failed to load: " << e.what();
      return;
   }
      
   /* xxx: this is all SUPER SUPER dangerous.  if any of these things cause lua
      to blow up, the process will exit(), since there's no protected handler installed!
      */
   object radiant = globals(L)["radiant"];
   object gs = radiant["gamestate"];
   object create_player = gs["_create_player"];
   if (type(create_player) == LUA_TFUNCTION) {
      p1_ = luabind::call_function<object>(create_player, 'civ');
   }
   now_ = 0;
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
   if (1 || perf_counter_deferred_) {
      json::Node counters;
      //LOG_(0) << "------------------------------------------------------";
      for (const auto& entry : perf_counters_.GetCounters()) {
         json::Node row;
         row.set("name", entry.first);
         row.set("value", static_cast<int>(entry.second.GetValue())); // xxx: counters need a type...
         //LOG_(0) << " " << std::setw(32) << entry.first << " : " << entry.second.GetValue();
      }
      if (perf_counter_deferred_) {
         perf_counter_deferred_->Notify(counters);
      }
   }
}


void Simulation::StepPathFinding()
{
   platform::timer t(1000);
   radiant::stdutil::ForEachPrune<Job>(jobs_, [&](std::shared_ptr<Job> &p) {
      if (!p->IsFinished() && !p->IsIdle()) {
         p->Work(t);
         SIM_LOG(5) << p->GetProgress();
      }
   });
}

void Simulation::Save(std::string id)
{
   ASSERT(false);
}

void Simulation::Load(std::string id)
{  
   ASSERT(false);
}

void Simulation::FireLuaTraces()
{
   MEASURE_TASK_TIME("lua")

   SIM_LOG_GAMELOOP(7) << "firing lua traces";
   lua_traces_->Flush();
}

void Simulation::UpdateGameState()
{
   MEASURE_TASK_TIME("lua")

   // Run AI...
   SIM_LOG_GAMELOOP(7) << "calling lua update";
   try {
      int interval = static_cast<int>(game_speed_ * game_tick_interval_);
      clock_->SetTime(clock_->GetTime() + interval);
      now_ = luabind::call_function<int>(game_api_["update"], profile_next_lua_update_);      
   } catch (std::exception const& e) {
      SIM_LOG(3) << "fatal error initializing game update: " << e.what();
      GetScript().ReportCStackThreadException(GetScript().GetCallbackThread(), e);
   }
   profile_next_lua_update_ = false;
}

void Simulation::UpdateCollisions()
{
   MEASURE_TASK_TIME("collisions")

   // Collision detection...
   SIM_LOG_GAMELOOP(7) << "updating collision system";
   GetOctTree().Update(now_);
}

void Simulation::EncodeBeginUpdate(std::shared_ptr<RemoteClient> c)
{
   proto::Update update;
   update.set_type(proto::Update::BeginUpdate);
   auto msg = update.MutableExtension(proto::BeginUpdate::extension);
   msg->set_sequence_number(sequence_number_);
   c->send_queue->Push(protocol::Encode(update));
}

void Simulation::EncodeEndUpdate(std::shared_ptr<RemoteClient> c)
{
   proto::Update update;

   update.set_type(proto::Update::EndUpdate);
   c->send_queue->Push(protocol::Encode(update));
}

void Simulation::EncodeServerTick(std::shared_ptr<RemoteClient> c)
{
   proto::Update update;

   update.set_type(proto::Update::SetServerTick);
   auto msg = update.MutableExtension(proto::SetServerTick::extension);
   msg->set_now(now_);
   msg->set_interval(net_send_interval_);
   msg->set_next_msg_time(net_send_interval_);
   SIM_LOG_GAMELOOP(7) << "sending server tick " << now_;
   c->send_queue->Push(protocol::Encode(update));
}

void Simulation::EncodeUpdates(std::shared_ptr<RemoteClient> c)
{
   c->streamer->Flush();
   EncodeDebugShapes(c->send_queue);

   for (const auto& msg : buffered_updates_) {
      c->send_queue->Push(msg);
   }
   buffered_updates_.clear();
}

phys::OctTree &Simulation::GetOctTree()
{
   return *octtree_;
}

om::EntityPtr Simulation::CreateEntity()
{
   om::EntityPtr entity = GetStore().AllocObject<om::Entity>();
   entityMap_[entity->GetObjectId()] = entity;
   return entity;
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
      i->second->Destroy();
      entityMap_.erase(i);
   }
}


dm::Store& Simulation::GetStore()
{
   return store_;
}


void Simulation::AddTask(std::shared_ptr<Task> task)
{
   tasks_.push_back(task);
}

void Simulation::AddJob(std::shared_ptr<Job> job)
{
   jobs_.push_back(job);
}

void Simulation::AddJobForEntity(om::EntityPtr entity, PathFinderPtr pf)
{
   if (entity) {
      dm::ObjectId id = entity->GetObjectId();
      auto i = entity_jobs_schedulers_.find(id);
      if (i == entity_jobs_schedulers_.end()) {
         EntityJobSchedulerPtr ejs = std::make_shared<EntityJobScheduler>(*this, entity);
         i = entity_jobs_schedulers_.insert(make_pair(id, ejs)).first;
         jobs_.push_back(ejs);
      }
      i->second->RegisterPathfinder(pf);
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
   if (debug_navgrid_enabled_) {
      GetOctTree().ShowDebugShapes(debug_navgrid_point_, msg);
   }
   queue->Push(protocol::Encode(update));
}

void Simulation::ProcessTaskList()
{
   MEASURE_TASK_TIME("native tasks")
   SIM_LOG_GAMELOOP(7) << "processing task list";

   auto i = tasks_.begin();
   while (i != tasks_.end()) {
      std::shared_ptr<Task> task = i->lock();

      if (task && task->Work(game_loop_timer_)) {
         i++;
      } else {
         i = tasks_.erase(i);
      }
   }
}

void Simulation::ProcessJobList()
{
   MEASURE_TASK_TIME("pathfinder")

   // Pahtfinders down here...
   if (_singleStepPathFinding) {
      SIM_LOG(5) << "skipping job processing (single step is on).";
      return;
   }
   SIM_LOG_GAMELOOP(7) << "processing job list";

   int idleCountdown = jobs_.size();
   while (!game_loop_timer_.expired() && !jobs_.empty() && idleCountdown) {
      std::weak_ptr<Job> front = radiant::stdutil::pop_front(jobs_);
      std::shared_ptr<Job> job = front.lock();
      if (job) {

         if (!job->IsFinished()) {
            if (!job->IsIdle()) {
               idleCountdown = jobs_.size() + 2;
               job->Work(game_loop_timer_);
               if (LOG_IS_ENABLED(simulation.jobs, 7)) {
                  LOG(simulation.jobs, 7) << job->GetProgress();
               }
            }
            jobs_.push_back(front);
         } else {
            SIM_LOG_GAMELOOP(5) << "destroying job..";
         }
      }
      idleCountdown--;
   }
   SIM_LOG_GAMELOOP(7) << "done processing job list";
}

void Simulation::PostCommand(proto::PostCommandRequest const& request)
{
   protobuf_reactor_->Dispatch(session_, request);
}

bool Simulation::ProcessMessage(std::shared_ptr<RemoteClient> c, const proto::Request& msg)
{
#define DISPATCH_MSG(MsgName) \
   case proto::Request::MsgName ## Request: \
      MsgName(msg.GetExtension(proto::MsgName ## Request::extension)); \
      break;

   switch (msg.type()) {
      DISPATCH_MSG(PostCommand)
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

perfmon::Store& Simulation::GetPerfmonCounters()
{
   return perf_counters_;
}


void Simulation::InitDataModel()
{
   object_model_traces_ = std::make_shared<dm::TracerSync>("sim objects");
   pathfinder_traces_ = std::make_shared<dm::TracerSync>("sim pathfinder");
   lua_traces_ = std::make_shared<dm::TracerBuffered>("sim lua", store_);  

   store_.AddTracer(lua_traces_, dm::LUA_ASYNC_TRACES);
   store_.AddTracer(object_model_traces_, dm::LUA_SYNC_TRACES);
   store_.AddTracer(object_model_traces_, dm::OBJECT_MODEL_TRACES);
   store_.AddTracer(pathfinder_traces_, dm::PATHFINDER_TRACES);
   store_trace_ = store_.TraceStore("sim")->OnAlloced([=](dm::ObjectPtr obj) {
      if (obj->GetObjectId() == 1) {
         GetOctTree().SetRootEntity(std::dynamic_pointer_cast<om::Entity>(obj));
         store_trace_ = nullptr;
      }
   });

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
      c->send_queue = protocol::SendQueue::Create(*socket);
      c->recv_queue = std::make_shared<protocol::RecvQueue>(*socket);
      c->streamer = std::make_shared<dm::Streamer>(store_, dm::PLAYER_1_TRACES, c->send_queue.get());
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
   CreateNew();

   unsigned int last_stat_dump = 0;

   net_send_timer_.set(0); // the first update gets pushed out immediately
   game_loop_timer_.set(game_tick_interval_);
   while (1) {
      Mainloop();
   }
}

void Simulation::Mainloop()
{
   perf_timeline_.BeginFrame();

   SIM_LOG_GAMELOOP(7) << "starting next gameloop";
   ReadClientMessages();

   if (!paused_) {
      lastNow_ = now_;
      UpdateGameState();
      UpdateCollisions();
      ProcessTaskList();
      ProcessJobList();
      FireLuaTraces();
   }
   if (next_counter_push_.expired()) {
      PathFinder::ComputeCounters(perf_counters_);
      PushPerformanceCounters();
      next_counter_push_.set(500);
   }
   // Disable the independant netsend timer until I can figure out this clock synchronization stuff -- tonyc
   static const bool disable_net_send_timer = true;
   if (disable_net_send_timer || net_send_timer_.expired()) {
      SendClientUpdates();
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
   //LuaGC();
   Idle();
}

void Simulation::LuaGC()
{
   MEASURE_TASK_TIME("lua gc")
   SIM_LOG_GAMELOOP(7) << "starting lua gc";
   scriptHost_->GC(game_loop_timer_);
}

void Simulation::Idle()
{
   // use of advance considered harmful.  what if the server gets "stuck"?  (e.g. in the debugger)
   if (!noidle_) {
      MEASURE_TASK_TIME("idle")
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

void Simulation::SendClientUpdates()
{
   MEASURE_TASK_TIME("network send")
   SIM_LOG_GAMELOOP(7) << "sending client updates (time left on net timer: " << net_send_timer_.remaining() << ")";

   for (std::shared_ptr<RemoteClient> c : _clients) {
      SendUpdates(c);
      protocol::SendQueue::Flush(c->send_queue);
   };
}

void Simulation::SendUpdates(std::shared_ptr<RemoteClient> c)
{

   EncodeBeginUpdate(c);
   EncodeServerTick(c);
   EncodeUpdates(c);
   EncodeEndUpdate(c);
}

void Simulation::ReadClientMessages()
{
   MEASURE_TASK_TIME("network recv")
   SIM_LOG_GAMELOOP(7) << "processing messages";

   _io_service->poll();
   _io_service->reset();

   for (auto c : _clients) {
      c->recv_queue->Process<proto::Request>([=](proto::Request const& msg) -> bool {
         return ProcessMessage(c, msg);
      });
   }
}

float Simulation::GetBaseWalkSpeed() const
{
   return base_walk_speed_ * game_speed_;
}
