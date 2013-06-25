#include "pch.h"
#include "platform/utils.h"
#include "simulation.h"
#include "metrics.h"
#include "jobs/multi_path_finder.h"
#include "jobs/path_finder.h"
#include "jobs/follow_path.h"
#include "jobs/goto_location.h"
#include "script/script_host.h"
#include "resources/res_manager.h"
#include "dm/store.h"
#include "om/entity.h"
#include "om/components/clock.h"
#include "om/components/mob.h"
#include "om/components/terrain.h"
#include "om/components/entity_container.h"
#include "om/components/grid_collision_shape.h"
#include "om/components/render_grid.h"
#include "om/components/build_orders.h"
#include "om/components/aura_list.h"
#include "om/components/target_tables.h"
#include "om/object_formatter/object_formatter.h"
#include "native_commands/create_room_cmd.h"
#include "jobs/job.h"
#include <boost/algorithm/string.hpp>

static const int __initialCivCount = 3;

using namespace ::radiant;
using namespace ::radiant::simulation;

namespace proto = ::radiant::tesseract::protocol;

Simulation* Simulation::singleton_;

Simulation& Simulation::GetInstance()
{
   ASSERT(singleton_ != nullptr);
   return *singleton_;
}

Simulation::Simulation(lua_State* L) :
   _showDebugNodes(false),
   _singleStepPathFinding(false),
   L_(L),
   store_(1)
{
   singleton_ = this;
   octtree_ = std::unique_ptr<Physics::OctTree>(new Physics::OctTree());

   commands_["radiant.toggle_debug_nodes"] = std::bind(&Simulation::ToggleDebugShapes, this, std::placeholders::_1);
   commands_["radiant.toggle_step_paths"] = std::bind(&Simulation::ToggleStepPathFinding, this, std::placeholders::_1);
   commands_["radiant.step_paths"] = std::bind(&Simulation::StepPathFinding, this, std::placeholders::_1);
   commands_["create_room"] = [](const proto::DoAction& msg) { return CreateRoomCmd()(msg); };

   om::RegisterObjectTypes(store_);
}

Simulation::~Simulation()
{
   // Control the order that items get destroyed for graceful shutdown.
   // xxx: this doesnt' work at all.  use std::shared_ptrs<> to keep things alive
   // (really only done by the simulation manager) and std::weak_ptrs<> to hold
   // references to things, with explicit lifetime registration callbacks
   // in the new Storage system if you REALLY care. (xxx)
   octtree_.release();
   singleton_ = nullptr;
}

void Simulation::CreateNew()
{
   ASSERT(this == &Simulation::GetInstance());

   guards_ += store_.TraceDynamicObjectAlloc(std::bind(&Simulation::OnObjectAllocated, this, std::placeholders::_1));

   scripts_.reset(new ScriptHost(L_));
   scripts_->CreateNew();
   now_ = 0;
}

std::string Simulation::ToggleDebugShapes(const proto::DoAction& msg)
{
   _showDebugNodes = !_showDebugNodes;
   LOG(WARNING) << "debug nodes turned " << (_showDebugNodes ? "ON" : "OFF");
   return "";
}

std::string Simulation::ToggleStepPathFinding(const proto::DoAction& msg)
{
   _singleStepPathFinding = !_singleStepPathFinding;
   LOG(WARNING) << "single step path finding turned " << (_singleStepPathFinding ? "ON" : "OFF");
   return "";
}

std::string Simulation::StepPathFinding(const proto::DoAction& msg)
{
   platform::timer t(1000);
   radiant::stdutil::ForEachPrune<Job>(_pathFinders, [&](std::shared_ptr<Job> &p) {
      if (!p->IsFinished() && !p->IsIdle(now_)) {
         p->Work(now_, t);
         p->LogProgress(LOG(WARNING));
      }
   });
   return "";
}

void Simulation::ProcessCommand(const proto::Cmd &cmd)
{
   ASSERT(false);
}

void Simulation::ProcessCommand(::google::protobuf::RepeatedPtrField<proto::Reply >* replies, const proto::Command &command)
{
   ASSERT(false);
}

void Simulation::Save(std::string id)
{
   ASSERT(false);
}

void Simulation::Load(std::string id)
{  
   ASSERT(false);
}

void Simulation::Step(platform::timer &timer, int interval)
{
   PROFILE_BLOCK();
   lastNow_ = now_;

   //IncrementClock(interval);

   // Run AI...
   scripts_->Update(interval, now_);

   // Collision detection...
   GetOctTree().Update(now_);
   UpdateAuras(now_);
   UpdateTargetTables(now_, now_ - lastNow_);

   // Send out change notifications
   store_.FireTraces();

   // One last opportunity for the script layer to do something.
   // Some objects may accumulate state when traces fire (e.g.
   // setting dirty bits).  This gives them an opportunity to
   // actually change state before we push a change over the
   // network or start doing some heavy lifting (like pathfinding
   // jobs).
   scripts_->CallGameHook("post_trace_firing");

   // Run jobs with the time left over
   ProcessJobList(now_, timer);
}

void Simulation::Idle(platform::timer &timer)
{
   scripts_->Idle(timer);
}

void Simulation::EncodeUpdates(protocol::SendQueuePtr queue, ClientState& cs)
{
   proto::Update update;

   // Set the server tick...
   update.set_type(proto::Update::SetServerTick);
   auto msg = update.MutableExtension(proto::SetServerTick::extension);
   msg->set_now(now_);
   queue->Push(protocol::Encode(update));

   // Process all objects which have been modified since 
   if (!allocated_.empty()) {
      update.Clear();
      update.set_type(proto::Update::AllocObjects);
      auto msg = update.MutableExtension(proto::AllocObjects::extension);
      for (const auto& o : allocated_) {
         auto allocMsg = msg->add_objects();
         allocMsg->set_object_id(o.first);
         allocMsg->set_object_type(o.second);
      }
      allocated_.clear();
      queue->Push(protocol::Encode(update));
   }

   if (!destroyed_.empty()) {
      update.Clear();
      update.set_type(proto::Update::RemoveObjects);
      auto msg = update.MutableExtension(proto::RemoveObjects::extension);
      for (dm::ObjectId id : destroyed_) {
         msg->add_objects(id);
      }
      destroyed_.clear();
      queue->Push(protocol::Encode(update));
   }

   auto modified = store_.GetModifiedSince(cs.last_update);
   std::sort(modified.begin(), modified.end());

   // The delta tracking of saved objects can keep deleted objects alive temporarily.
   // For example, if the last referece to a std::shared_ptr<Foo> is in the removed list
   // of dm::Set bar, that object won't be reclaimed until we push the changes for
   // bar.  If Foo is also in the modified set, the object id for Foo will reference
   // freed memory.
   for (dm::ObjectId id : modified) {
      dm::Object* obj = store_.FetchStaticObject(id);
      if (obj) {
         update.Clear();
         update.set_type(proto::Update::UpdateObject);
         auto msg = update.MutableExtension(proto::UpdateObject::extension);

         obj->SaveObject(msg->mutable_object());
         queue->Push(protocol::Encode(update));
      }
   }
   EncodeDebugShapes(queue);

   cs.last_update = store_.GetNextGenerationId();
}

void Simulation::SendCommandReply(proto::Reply* reply)
{
}

Physics::OctTree &Simulation::GetOctTree()
{
   return *octtree_;
}

om::EntityPtr Simulation::GetRootEntity()
{
   return scripts_->GetEntity(1).lock();
}

dm::Store& Simulation::GetStore()
{
   return store_;
}


std::shared_ptr<MultiPathFinder> Simulation::CreateMultiPathFinder(std::string name)
{
   std::shared_ptr<MultiPathFinder> path = std::make_shared<MultiPathFinder>(name);
   _pathFinders.push_back(path);
   return path;
}

std::shared_ptr<PathFinder> Simulation::CreatePathFinder(std::string name, om::EntityRef entity, luabind::object solved, luabind::object dst_filter)
{
   std::shared_ptr<PathFinder> pf = std::make_shared<PathFinder>(name, entity, solved, dst_filter);
   _pathFinders.push_back(pf);
   return pf;
}

std::shared_ptr<FollowPath> Simulation::CreateFollowPath(om::EntityRef entity, float speed, std::shared_ptr<Path> path, float close_to_distance)
{
   std::shared_ptr<FollowPath> fp = std::make_shared<FollowPath>(entity, speed, path, close_to_distance);
   _loopTasks.push_back(fp);
   return fp;
}

std::shared_ptr<GotoLocation> Simulation::CreateGotoLocation(om::EntityRef entity, float speed, const math3d::point3& location, float close_to_distance)
{
   std::shared_ptr<GotoLocation> fp = std::make_shared<GotoLocation>(entity, speed, location, close_to_distance);
   _loopTasks.push_back(fp);
   return fp;
}

std::shared_ptr<GotoLocation> Simulation::CreateGotoEntity(om::EntityRef entity, float speed, om::EntityRef target, float close_to_distance)
{
   std::shared_ptr<GotoLocation> fp = std::make_shared<GotoLocation>(entity, speed, target, close_to_distance);
   _loopTasks.push_back(fp);
   return fp;
}

JSONNode Simulation::FormatObjectAtUri(std::string const& uri)
{
   std::vector<std::string> parts;
   boost::algorithm::split(parts, uri, boost::is_any_of("/"));

   // should be "", "object", "13"...
   if (parts.size() > 2) {
      dm::ObjectId id;
      std::stringstream(parts[2]) >> id;

      dm::ObjectPtr obj = store_.FetchObject<dm::Object>(id);
      if (obj) {
         return om::ObjectFormatter("/object/").ObjectToJson(obj);
      }
   }
   return JSONNode(); // actually, return an error!
}

void Simulation::FetchObject(proto::FetchObjectRequest const& request, proto::FetchObjectReply* reply)
{
   JSONNode node = FormatObjectAtUri(request.uri());
   reply->set_json(node.write());
}

void Simulation::DoAction(const proto::DoAction& msg, protocol::SendQueuePtr queue)
{
   std::string json;

   auto i = commands_.find(msg.action());
   if (i != commands_.end()) {
      json = i->second(msg);
   } else {
      json = scripts_->DoAction(msg);
   }
   if (msg.has_reply_id()) {
      proto::Update update;
      update.set_type(proto::Update::DoActionReply);
      auto reply = update.MutableExtension(proto::DoActionReply::extension);

      reply->set_reply_id(msg.reply_id());
      if (json.empty()) {
         json = "{}";
      }
      reply->set_json(json);

      queue->Push(protocol::Encode(update));
   }
}

void Simulation::EncodeDebugShapes(protocol::SendQueuePtr queue)
{
   proto::Update update;
   update.set_type(proto::Update::UpdateDebugShapes);
   auto uds = update.MutableExtension(proto::UpdateDebugShapes::extension);
   auto msg = uds->mutable_shapelist();

   if (_showDebugNodes) {
      radiant::stdutil::ForEachPrune<Job>(_loopTasks, [&](std::shared_ptr<Job> &job) {
         job->EncodeDebugShapes(msg);
      });
      radiant::stdutil::ForEachPrune<Job>(_pathFinders, [&](std::shared_ptr<Job> &job) {
         job->EncodeDebugShapes(msg);
      });
   }

   queue->Push(protocol::Encode(update));
}


void Simulation::ProcessJobList(int now, platform::timer &timer)
{
   PROFILE_BLOCK();

   auto i = _loopTasks.begin();
   while (i != _loopTasks.end()) {
      std::shared_ptr<Job> task = i->lock();
      if (task && !task->IsFinished()) {
         task->Work(now, timer);
         i++;
      } else {
         i = _loopTasks.erase(i);
      }
   }

   // Pahtfinders down here...
   if (_singleStepPathFinding) {
      LOG(INFO) << "skipping job processing (single step is on).";
      return;
   }


   int idleCountdown = _pathFinders.size();
   LOG(INFO) << timer.remaining() << " ms remaining in process job list (" << idleCountdown << " jobs).";

   while (!timer.expired() && !_pathFinders.empty() && idleCountdown) {
      std::weak_ptr<Job> front = radiant::stdutil::pop_front(_pathFinders);
      std::shared_ptr<Job> job = front.lock();
      if (job) {
         if (!job->IsFinished()) {
            if (!job->IsIdle(now)) {
               idleCountdown = _pathFinders.size() + 2;
               job->Work(now, timer);
            }
            //job->LogProgress(LOG(INFO));
            _pathFinders.push_back(front);
         } else {
            LOG(WARNING) << "destroying job..";
         }
      }
      idleCountdown--;
   }
}

void Simulation::OnObjectAllocated(dm::ObjectPtr obj)
{
   dm::ObjectId id = obj->GetObjectId();

   ASSERT(obj->GetObjectType() >= 1000);

   // LOG(WARNING) << "tracking object allocate: " << id;

   allocated_.push_back(std::make_pair(obj->GetObjectId(), obj->GetObjectType()));
   guards_ += obj->TraceObjectLifetime("simulation entity lifetime", std::bind(&Simulation::OnObjectDestroyed, this, id));
   
   if (obj->GetObjectType() == om::EntityObjectType) {
      TraceEntity(std::static_pointer_cast<om::Entity>(obj));
   }
}

void Simulation::OnObjectDestroyed(dm::ObjectId id)
{
   int c = allocated_.size();
   for (int i = 0; i < c; i++) {
      if (allocated_[i].first == id) {
         // LOG(WARNING) << "tracking object destroy (removing from allocated_): " << id;
         allocated_[i] = allocated_[--c];
         allocated_.resize(c);
         return;
      }
   }
   // LOG(WARNING) << "tracking object destroy: " << id;
   destroyed_.push_back(id);
}


void Simulation::TraceEntity(om::EntityPtr entity)
{
   if (entity->GetObjectId() == 1) {
      octtree_->SetRootEntity(entity);
   }

   const auto& components = entity->GetComponents();
   guards_ += components.TraceMapChanges("simulation component checking",
                                          std::bind(&Simulation::ComponentAdded, this, om::EntityRef(entity), std::placeholders::_1, std::placeholders::_2),
                                          nullptr);
   for (const auto& entry : components) {
      ComponentAdded(entity, entry.first, entry.second);
   }
}

void Simulation::ComponentAdded(om::EntityRef e, dm::ObjectType type, std::shared_ptr<dm::Object> component)
{
   auto entity = e.lock();
   if (entity) {
      switch (type) {
      case om::AuraListObjectType: {
         om::AuraListPtr list = std::static_pointer_cast<om::AuraList>(component);
         const auto& auras = list->GetAuras();
         guards_ += auras.TraceSetChanges("sim update auras",
                                          std::bind(&Simulation::TraceAura, this, om::AuraListRef(list), std::placeholders::_1),
                                          nullptr);
         for (om::AuraPtr aura : auras) {
            TraceAura(list, aura);
         }
         break;
      }
      case om::TargetTablesObjectType: {
         TraceTargetTables(std::static_pointer_cast<om::TargetTables>(component));
         break;
      }
      }
   }
}

void Simulation::UpdateAuras(int now)
{
   int i = 0, c = auras_.size();

   while (i < c) {
      AuraListEntry &entry = auras_[i];
      auto aura = entry.aura.lock();
      if (aura) {
         int expires = aura->GetExpireTime();
         if (expires && expires < now) {
            auto list = entry.list.lock();
            list->RemoveAura(aura);

            auto L = scripts_->GetInterpreter();
            luabind::object handler = aura->GetMsgHandler();
            luabind::object md = luabind::globals(L)["md"];
            luabind::object aobj(L, aura);
            luabind::call_function<void>(md["send_msg"], md, handler, "radiant.events.aura_expired", aobj);
            aura = nullptr;
         }         
      } 
      if (aura) {
         i++;
      } else {
         auras_[i] = auras_[--c];
         auras_.resize(c);
      }
   }
}

void Simulation::TraceAura(om::AuraListRef list, om::AuraPtr aura)
{
   // xxx - this is annoying... can we avoid the dup checking with
   // smarter trace firing code?
   for (const AuraListEntry& entry : auras_) {
      if (aura == entry.aura.lock()) {
         return;
      }
   }
   auras_.push_back(AuraListEntry(list, aura));
}

void Simulation::TraceTargetTables(om::TargetTablesPtr tables)
{
   // xxx - this is annoying... can we avoid the dup checking with
   // smarter trace firing code?
   for (const om::TargetTablesRef& t : targetTables_) {
      if (tables == t.lock()) {
         return;
      }
   }
   targetTables_.push_back(tables);
}

void Simulation::UpdateTargetTables(int now, int interval)
{
   int i = 0, c = targetTables_.size();
   for (i = 0; i < c; i++) {
      om::TargetTablesPtr table = targetTables_[i].lock();
      if (table) {
         table->Update(now, interval);
         i++;
      } else {
         targetTables_[i] = targetTables_[--c];
         targetTables_.resize(c);
      }
   }
}
