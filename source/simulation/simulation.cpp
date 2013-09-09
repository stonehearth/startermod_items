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
#include "om/components/aura_list.h"
#include "om/components/target_tables.h"
#include "om/components/lua_components.h"
#include "om/object_formatter/object_formatter.h"
#include "om/data_binding.h"
//#include "native_commands/create_room_cmd.h"
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
   _showDebugNodes(true),
   _singleStepPathFinding(false),
   L_(L),
   store_(1)
{
   singleton_ = this;
   octtree_ = std::unique_ptr<Physics::OctTree>(new Physics::OctTree());
   scriptHost_.reset(new ScriptHost(L_));

   commands_["radiant.toggle_debug_nodes"] = std::bind(&Simulation::ToggleDebugShapes, this, std::placeholders::_1);
   commands_["radiant.toggle_step_paths"] = std::bind(&Simulation::ToggleStepPathFinding, this, std::placeholders::_1);
   commands_["radiant.step_paths"] = std::bind(&Simulation::StepPathFinding, this, std::placeholders::_1);
   //commands_["create_room"] = [](const proto::DoAction& msg) { return CreateRoomCmd()(msg); };

   auto& rm = resources::ResourceManager2::GetInstance();
   for (std::string const& modname : rm.GetModuleNames()) {
      try {
         json::ConstJsonObject manifest = rm.LookupManifest(modname);
         json::ConstJsonObject const& block = manifest["server"];
         LOG(WARNING) << "loading init script for " << modname << "...";
         LoadModuleInitScript(block);
         LoadModuleRequestHandlers(modname, block);
         LoadModuleGameObjects(modname, block);
      } catch (std::exception const& e) {
         LOG(WARNING) << "load failed: " << e.what();
      }
   }
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


void Simulation::LoadModuleInitScript(json::ConstJsonObject const& block)
{
   try {
      std::string filename = block["init_script"].as_string();
      scriptHost_->LuaRequire(filename);
   } catch (std::exception const& e) {
      LOG(WARNING) << "load failed: " << e.what();
   }
}

static std::string SanatizePath(std::string const& path)
{
   std::vector<std::string> s;
   boost::split(s, path, boost::is_any_of("/"));
   s.erase(std::remove(s.begin(), s.end(), ""), s.end());
   return std::string("/") + boost::algorithm::join(s, "/");
}

void Simulation::LoadModuleRequestHandlers(std::string const& modname, json::ConstJsonObject const& block)
{
   resources::ResourceManager2 &rm = resources::ResourceManager2::GetInstance();
   try {
      for (auto const& node : block["request_handlers"]) {
         std::string const& uri_path = SanatizePath("/modules/server/" + modname + "/" + node.name());
         std::string const& lua_path = node.as_string();
         routes_[uri_path] = scriptHost_->LuaRequire(lua_path);
      }
   } catch (std::exception const& e) {
      LOG(WARNING) << "load failed: " << e.what();
   }
}

void Simulation::LoadModuleGameObjects(std::string const& modname, json::ConstJsonObject const& block)
{
   resources::ResourceManager2 &rm = resources::ResourceManager2::GetInstance();
   try {
      for (auto const& node : block["game_objects"]) {
         json::ConstJsonObject info(node);

         std::string dataBindingName = modname + std::string(".") + node.name();
         std::string controller = info["controller"].as_string();
         dataBindings_[dataBindingName] = scriptHost_->LuaRequire(controller);
         if (info.has("publish_at")) {
            om::DataBindingPtr binding = GetStore().AllocObject<om::DataBinding>();
            binding->SetDataObject(luabind::newtable(scriptHost_->GetInterpreter()));
            luabind::object ctor = dataBindings_[dataBindingName];
            luabind::object model = luabind::call_function<luabind::object>(ctor, binding);
            std::ostringstream uri;
            uri << "/server/objects/" << modname << "/" << info["publish_at"].as_string();
            RegisterServerRemoteObject(uri.str(), binding);
         }
      }
   } catch (std::exception const& e) {
      LOG(WARNING) << "load failed: " << e.what();
   }
}

void Simulation::CreateNew()
{
   ASSERT(this == &Simulation::GetInstance());

   guards_ += store_.TraceDynamicObjectAlloc(std::bind(&Simulation::OnObjectAllocated, this, std::placeholders::_1));

   scriptHost_->CreateNew();
   now_ = 0;
}

std::string Simulation::ToggleDebugShapes(std::string const& cmd)
{
   _showDebugNodes = !_showDebugNodes;
   LOG(WARNING) << "debug nodes turned " << (_showDebugNodes ? "ON" : "OFF");
   return "";
}

std::string Simulation::ToggleStepPathFinding(std::string const& cmd)
{
   _singleStepPathFinding = !_singleStepPathFinding;
   LOG(WARNING) << "single step path finding turned " << (_singleStepPathFinding ? "ON" : "OFF");
   return "";
}

std::string Simulation::StepPathFinding(std::string const& cmd)
{
   platform::timer t(1000);
   radiant::stdutil::ForEachPrune<Job>(jobs_, [&](std::shared_ptr<Job> &p) {
      if (!p->IsFinished() && !p->IsIdle()) {
         p->Work(t);

         std::ostringstream progress;
         p->LogProgress(progress);
         LOG(WARNING) << progress.str();
      }
   });
   return "";
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
   scriptHost_->Update(interval, now_);

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
   scriptHost_->CallGameHook("post_trace_firing");

   // Run jobs with the time left over
   ProcessJobList(timer);
}

void Simulation::Idle(platform::timer &timer)
{
   scriptHost_->Idle(timer);
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
   PushServerRemoteObjects(queue);

   cs.last_update = store_.GetNextGenerationId();
}

Physics::OctTree &Simulation::GetOctTree()
{
   return *octtree_;
}

om::EntityPtr Simulation::GetRootEntity()
{
   return scriptHost_->GetEntity(1).lock();
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

void Simulation::ScriptCommand(tesseract::protocol::ScriptCommandRequest const& request, tesseract::protocol::ScriptCommandReply* reply)
{
   std::string result;
   std::string const& cmd = request.cmd();

   auto i = commands_.find(cmd);
   if (i != commands_.end()) {
      result = i->second(cmd);
   }
   if (reply) {
      reply->set_result(result);
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

   queue->Push(protocol::Encode(update));
}

void Simulation::PushServerRemoteObjects(protocol::SendQueuePtr queue)
{
   if (!serverRemoteObjects_.empty()) {
      proto::Update update;
      update.set_type(proto::Update::DefineRemoteObject);
      auto drs = update.MutableExtension(proto::DefineRemoteObject::extension);
      for (const auto& entry : serverRemoteObjects_) {
         drs->set_uri(entry.first);
         drs->set_object_uri(entry.second);
         queue->Push(protocol::Encode(update));
      }
      serverRemoteObjects_.clear();
   }
}


void Simulation::ProcessJobList(platform::timer &timer)
{
   PROFILE_BLOCK();

   auto i = tasks_.begin();
   while (i != tasks_.end()) {
      std::shared_ptr<Task> task = i->lock();
      if (task && task->Work(timer)) {
         i++;
      } else {
         i = tasks_.erase(i);
      }
   }

   // Pahtfinders down here...
   if (_singleStepPathFinding) {
      LOG(INFO) << "skipping job processing (single step is on).";
      return;
   }


   int idleCountdown = jobs_.size();
   LOG(INFO) << timer.remaining() << " ms remaining in process job list (" << idleCountdown << " jobs).";

   while (!timer.expired() && !jobs_.empty() && idleCountdown) {
      std::weak_ptr<Job> front = radiant::stdutil::pop_front(jobs_);
      std::shared_ptr<Job> job = front.lock();
      if (job) {
         if (!job->IsFinished()) {
            if (!job->IsIdle()) {
               idleCountdown = jobs_.size() + 2;
               job->Work(timer);
               //job->LogProgress(LOG(WARNING));
            }
            jobs_.push_back(front);
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

            auto L = scriptHost_->GetInterpreter();
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


void Simulation::FetchObject(proto::FetchObjectRequest const& request, proto::FetchObjectReply* reply)
{
   std::vector<std::string> parts;
   std::string const& uri = request.uri();

   boost::algorithm::split(parts, uri, boost::is_any_of("/"));

   // should be "", "object", "13"...
   if (parts.size() < 3) {
      reply->set_status_code(404);
      reply->set_content("unknown uri " + uri);
      return;

   }
   dm::ObjectId id;
   std::stringstream(parts[2]) >> id;

   dm::ObjectPtr obj = store_.FetchObject<dm::Object>(id);
   if (!obj) {
      std::ostringstream error;
      error << "No object exists with id " << id;
      reply->set_status_code(500);
      reply->set_content(error.str());
   }

   JSONNode node = om::ObjectFormatter("/object/").ObjectToJson(obj);
   reply->set_status_code(200);
   reply->set_content(node.write());
   reply->set_mime_type("application/json");
   return;
}

// xxx: Merge into a common route thingy...
void Simulation::HandleRouteRequest(luabind::object ctor, JSONNode const& query, std::string const& postdata, proto::PostCommandReply* reply)
{
   // convert the json query data to a lua table...
   try {
      using namespace luabind;
      lua_State* L = scriptHost_->GetCallbackState();
      object queryObj = scriptHost_->JsonToLua(query);
      object coder = globals(L)["radiant"]["json"];

      object obj = call_function<object>(ctor);
      object fn = obj["handle_request"];
      auto i = query.find("fn");
      if (i != query.end() && i->type() == JSON_STRING) {
         std::string fname = i->as_string();
         if (!fname.empty())  {
            fn = obj[fname];
         }
      }
#if 0
      // tony, remove this please!
      object postdataObj;
      if (!postdata.empty()) {
         postdataObj = scriptHost_->JsonToLua(libjson::parse(postdata));
      }
      object result = call_function<object>(fn, obj, queryObj, postdataObj);
      std::string json = call_function<std::string>(coder["encode"], result);
#else
      std::string json = scriptHost_->PostCommand(fn, obj, postdata);
#endif

      reply->set_status_code(200);
      reply->set_content(json);
      reply->set_mime_type("application/json");
   } catch (std::exception &e) {
      JSONNode result;
      result.push_back(JSONNode("error", e.what()));
      reply->set_status_code(500);
      reply->set_content(result.write());
      reply->set_mime_type("application/json");
   }
}

void Simulation::PostCommand(proto::PostCommandRequest const& request, proto::PostCommandReply* reply)
{
   std::string response, error;
   std::string const& path = request.path();
   std::string const& query = request.query();
   std::string const& data = request.data();

   auto i = routes_.find(path);
   if (i != routes_.end()) {
      JSONNode args;
      // xxx: what about data?  if it's empty, we're in trouble, right?
      if (!query.empty()) {
         args = libjson::parse(query); // xxx: what if this throws an exception?  we're going to die!!
      }
      HandleRouteRequest(i->second, args, data, reply);
      return;
   }

   dm::ObjectPtr obj = om::ObjectFormatter("/object/").GetObject(GetStore(), request.path());
   if (obj) {
      if (obj->GetObjectType() == om::DataBindingObjectType) {
         JSONNode n = libjson::parse(query); // xxx: turn json::ConstJsonObject into a REAL WRAPPER instead of this const ref crap.
         json::ConstJsonObject node(n);
         try {
            om::DataBindingPtr db = std::static_pointer_cast<om::DataBinding>(obj);
            std::string fname = node["fn"].as_string();
            luabind::object obj = db->GetModelObject();
            luabind::object fn = obj[fname];
            response = scriptHost_->PostCommand(fn, obj, data);
         } catch (std::exception const& e) {
            error = e.what();
         }
      }
   } else {
      luabind::object obj = scriptHost_->LuaRequire(path);
      if (obj && luabind::type(obj) != LUA_TNIL) { // xxx: Just let the exception thing happen!
         response = scriptHost_->PostCommand(obj, data);
      } else {
         error = "no such object";
      }
   }

   reply->set_mime_type("application/json");
   reply->set_status_code(200); // xxx: or 500 or something?
   if (error.empty()) {
      reply->set_content(response);
   } else {
      JSONNode errorNode;
      errorNode.push_back(JSONNode("error", error));
      reply->set_content(errorNode.write());
   }
}

bool Simulation::ProcessMessage(const proto::Request& msg, protocol::SendQueuePtr queue)
{
   proto::Update reply;

#define DISPATCH_MSG(MsgName) \
   case proto::Request::MsgName ## Request: \
      reply.set_type(proto::Update::MsgName ## Reply); \
      reply.set_reply_id(msg.request_id()); \
      MsgName(msg.GetExtension(proto::MsgName ## Request::extension), \
              reply.MutableExtension(proto::MsgName ## Reply::extension)); \
      break;

   switch (msg.type()) {
      DISPATCH_MSG(FetchObject)
      DISPATCH_MSG(ScriptCommand)
      DISPATCH_MSG(PostCommand)
   default:
      ASSERT(false);
   }
   queue->Push(protocol::Encode(reply));
   return true;
}

void Simulation::RegisterServerRemoteObject(std::string const& uri, dm::ObjectPtr obj)
{
   std::pair<std::string, std::string> entry;

   entry.first = uri;
   entry.second = std::string("/object/") + stdutil::ToString(obj->GetObjectId());
   serverRemoteObjects_.push_back(entry);
}
