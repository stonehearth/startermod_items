#include "pch.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include "math3d/common/rect2d.h"
#include "selectors/actor_selector.h"
#include "selectors/voxel_range_selector.h"
#include "om/entity.h"
#include "om/components/terrain.h"
#include "om/selection.h"
#include "om/om_alloc.h"
#include "csg/lua/lua_csg.h"
#include "om/lua/lua_om.h"
#include "platform/utils.h"
#include "resources/res_manager.h"
#include "commands/command.h"
#include "commands/trace_entity.h"
#include "commands/create_room.h"
#include "commands/create_portal.h"
#include "commands/execute_action.h"
#include "renderer/render_entity.h"
#include "renderer/lua_render_entity.h"
#include "renderer/lua_renderer.h"
#include "lua/register.h"
#include "lua/script_host.h"
#include "om/stonehearth.h"
#include "om/object_formatter/object_formatter.h"
#include "deferred.h"
#include "glfw.h"

//  #include "GFx/AS3/AS3_Global.h"
#include "client.h"
#include "renderer/renderer.h"
#include "libjson.h"

using namespace ::radiant;
using namespace ::radiant::client;
namespace fs = boost::filesystem;
namespace po = boost::program_options;
namespace proto = ::radiant::tesseract::protocol;

DEFINE_SINGLETON(Client);

Client::Client() :
   _tcp_socket(_io_service),
   _send_ahead_commands(3),
   _server_last_update_time(0),
   _server_interval_duration(1),
   selectedObject_(NULL),
   showBuildOrders_(false),
   last_sequence_number_(-1),
   nextCmdId_(1),
   rootObject_(NULL),
   hwnd_(NULL),
   store_(2),
   authoringStore_(3),
   nextTraceId_(1),
   nextDeferredCommandId_(1),
   nextResponseHandlerId_(1),
   currentCommandCursor_(NULL),
   last_server_request_id_(0)
{
   om::RegisterObjectTypes(store_);
   om::RegisterObjectTypes(authoringStore_);

   std::vector<std::pair<dm::ObjectId, dm::ObjectType>>  allocated_;
}

Client::~Client()
{
   guards_.Clear();
   octtree_.release();
}

void Client::GetConfigOptions(po::options_description& options)
{
}


extern bool realtime;
void Client::run()
{
   LoadCursors();

   octtree_ = std::unique_ptr<Physics::OctTree>(new Physics::OctTree());
      
   Renderer& renderer = Renderer::GetInstance();
   hwnd_ = renderer.GetWindowHandle();
   //defaultCursor_ = (HCURSOR)GetClassLong(hwnd_, GCL_HCURSOR);

   renderer.SetMouseInputCallback(std::bind(&Client::OnMouseInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
   renderer.SetKeyboardInputCallback(std::bind(&Client::OnKeyboardInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
   SetRenderPipelineInfo();

   int width = renderer.GetWidth();
   int height = renderer.GetHeight();
   int debug_port = 1338;

   namespace po = boost::program_options;
   extern po::variables_map configvm;
   std::string loader = configvm["game.loader"].as<std::string>().c_str();
   json::ConstJsonObject manifest(resources::ResourceManager2::GetInstance().LookupManifest(loader));
   std::string docroot = "http://radiant/" + manifest["loader"]["ui"]["homepage"].as_string();

   browser_.reset(chromium::CreateBrowser(hwnd_, docroot, width, height, debug_port));
   browser_->SetCursorChangeCb(std::bind(&Client::SetUICursor, this, std::placeholders::_1));

   auto requestHandler = [=](std::string const& uri, JSONNode const& query, std::string const& postdata, std::shared_ptr<net::IResponse> response) {
      std::lock_guard<std::mutex> guard(lock_);

      if (boost::starts_with(uri, "/api/events")) {
         GetEvents(query, response);         
      } else {
         BrowserRequestHandler(uri, query, postdata, response);
      }
   };
   browser_->SetRequestHandler(requestHandler);

   auto onRawInput = [=](RawInputEvent const & re, bool& handled, bool& uninstall) {
      browser_->OnRawInput(re, handled, uninstall);
   };

   auto onMouse = [=](MouseEvent const & m, bool& handled, bool& uninstall) {
      browser_->OnMouseInput(m, handled, uninstall);
   };
   renderer.SetRawInputCallback(onRawInput);
   renderer.SetMouseInputCallback(onMouse);


   lua_State* L = scriptHost_->GetInterpreter();
   renderer.SetScriptHost(scriptHost_);
   lua::RegisterBasicTypes(L);
   om::RegisterLuaTypes(L);
   csg::RegisterLuaTypes(L);
   client::RegisterLuaTypes(L);
   luabind::globals(L)["_client"] = luabind::object(L, this);

   json::ConstJsonObject::RegisterLuaType(L);

   // this locks down the environment!  all types must be registered by now!!
   scriptHost_->LuaRequire("/radiant/client.lua");
   auto& rm = resources::ResourceManager2::GetInstance();
   for (std::string const& modname : rm.GetModuleNames()) {
      try {
         json::ConstJsonObject manifest = rm.LookupManifest(modname);
         json::ConstJsonObject const& block = manifest["client"];
         LOG(WARNING) << "loading init script for " << modname << "...";
         LoadModuleInitScript(block);
         LoadModuleRoutes(modname, block);
      } catch (std::exception const& e) {
         LOG(WARNING) << "load failed: " << e.what();
      }
   }

#if 0
   //_commands['S'] = std::bind(&Client::RunGlobalCommand, this, "create-stockpile", std::placeholders::_1);
   //_commands['F'] = std::bind(&Client::CreateFloor, this, std::placeholders::_1);

   _commands['V'] = std::bind(&Client::RotateViewMode, this, std::placeholders::_1);
   _commands['B'] = std::bind(&Client::ToggleBuildPlan, this, std::placeholders::_1);
   _commands['C'] = std::bind(&Client::CapStorey, this, std::placeholders::_1);
   _commands['E'] = std::bind(&Client::GrowWalls, this, std::placeholders::_1);
   _commands['R'] = std::bind(&Client::AddFixture, this, std::placeholders::_1, "door");
   _commands['L'] = std::bind(&Client::AddFixture, this, std::placeholders::_1, "wall-mounted-lantern");

   _mappedCommands["assign-worker"] = std::bind(&Client::AssignWorkerToBuilding, this, std::placeholders::_1);

   _commands[GLFW_KEY_F5] = std::bind(&Client::EvalCommand, this, "reset ai");
   _commands[GLFW_KEY_F6] = std::bind(&Client::EvalCommand, this, "new");
   _commands[GLFW_KEY_F10] = std::bind(&Client::FinishStructure, this, std::placeholders::_1);
   _commands[GLFW_KEY_F9] = [&](bool) { realtime = !realtime; LOG(WARNING) << "REALTIME IS " << realtime; return true; };
#endif

#if 0
   _commands[VK_NUMPAD1] = [&](bool) -> bool {
      auto *msg = _command_list.add_cmds();
      msg->set_id("save");
      return true;
   };
   _commands[VK_NUMPAD2] = [&](bool) -> bool {
      auto *msg = _command_list.add_cmds();
      msg->set_id("load");
      return true;
   };
#endif
   _commands[GLFW_KEY_F9] = std::bind(&Client::EvalCommand, this, "radiant.toggle_debug_nodes");
   _commands[GLFW_KEY_F3] = std::bind(&Client::EvalCommand, this, "radiant.toggle_step_paths");
   _commands[GLFW_KEY_F4] = std::bind(&Client::EvalCommand, this, "radiant.step_paths");
   _commands[GLFW_KEY_F5] = [=]() { ToggleBuildPlan(); };
   _commands[GLFW_KEY_ESC] = [=]() {
      currentTool_ = nullptr;
      currentCommand_ = nullptr;
      currentCommandCursor_ = NULL;
      currentToolName_ = "";
   };

   // _commands[VK_NUMPAD0] = std::shared_ptr<command>(new command_build_blueprint(*_proxy_manager, *_renderer, 500));

   setup_connections();

   _client_interval_start = platform::get_current_time_in_ms();
   _server_last_update_time = 0;

   while (renderer.IsRunning()) {
      static int last_stat_dump = 0;
      mainloop();
      int now = platform::get_current_time_in_ms();
      if (now - last_stat_dump > 10000) {
         // PROFILE_DUMP_STATS();
         // PROFILE_RESET();
         last_stat_dump = now;
      }
   }
}

void Client::LoadModuleInitScript(json::ConstJsonObject const& block)
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

void Client::LoadModuleRoutes(std::string const& modname, json::ConstJsonObject const& block)
{
   resources::ResourceManager2 &rm = resources::ResourceManager2::GetInstance();
   try {
      for (auto const& node : block["request_handlers"]) {
         std::string const& uri_path = SanatizePath("/modules/client/" + modname + "/" + node.name());
         std::string const& lua_path = node.as_string();
         clientRoutes_[uri_path] = scriptHost_->LuaRequire(lua_path);
      }
   } catch (std::exception const& e) {
      LOG(WARNING) << "load failed: " << e.what();
   }
}

void Client::handle_connect(const boost::system::error_code& error)
{
   if (error) {
      LOG(WARNING) << "connection to server failed (" << error << ").  retrying...";
      setup_connections();
   } else {
      recv_queue_ = std::make_shared<protocol::RecvQueue>(_tcp_socket);
      send_queue_ = protocol::SendQueue::Create(_tcp_socket);
      recv_queue_->Read();
   }
}

void Client::setup_connections()
{
   tcp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 8888);
   _tcp_socket.async_connect(endpoint, std::bind(&Client::handle_connect, this, std::placeholders::_1));
}

void Client::mainloop()
{
   std::lock_guard<std::mutex> guard(lock_);

   PROFILE_BLOCK();
   
   /*
    * Top of loop validation...
    */
   ASSERT(!(currentCommand_ && currentTool_));
   ASSERT(!(currentCommandCursor_ && !currentToolName_.empty()));

   process_messages();

   int currentTime = platform::get_current_time_in_ms();
   float alpha = (currentTime - _client_interval_start) / (float)_server_interval_duration;

   alpha = std::min(1.0f, std::max(alpha, 0.0f));
   now_ = (int)(_server_last_update_time + (_server_interval_duration * alpha));

   FlushEvents();
   if (browser_) {
      auto cb = [](const csg::Region2 &rgn, const char* buffer) {
         Renderer::GetInstance().UpdateUITexture(rgn, buffer);
      };
      browser_->Work();
      browser_->UpdateDisplay(cb);
   }

   InstallCursor();
   HilightMouseover();

   // Fire the authoring traces *after* pumping the chrome message loop, since
   // we may create or modify authoring objects as a result of input events
   // or calls from the browser.
   authoringStore_.FireTraces();

   Renderer::GetInstance().RenderOneFrame(now_, alpha);
   if (send_queue_) {
      protocol::SendQueue::Flush(send_queue_);
   }
}

om::TerrainPtr Client::GetTerrain()
{
   return rootObject_ ? rootObject_->GetComponent<om::Terrain>() : nullptr;
}

void Client::Reset()
{

   Renderer::GetInstance().Cleanup();
   octtree_->Cleanup();

   store_.Reset();

   //_proxy_manager->SetNotifyLoad(std::bind(&Client::OnObjectLoad, this, std::placeholders::_1));

   _server_last_update_time = 0;
   _server_interval_duration = 1;
   _client_interval_start = 0;

   rootObject_ = NULL;
   selectedObject_ = NULL;
   objects_.clear();
}

bool Client::ProcessRequestReply(const proto::Update& msg)
{
   int reply_id = msg.reply_id();
   if (reply_id == 0) {
      return false;
   }

   auto i = server_requests_.find(reply_id);
   if (i != server_requests_.end()) {
      i->second(msg);
      server_requests_.erase(i);
   }
   return true;
}

bool Client::ProcessMessage(const proto::Update& msg)
{
   if (ProcessRequestReply(msg)) {
      return true;
   }

#define DISPATCH_MSG(MsgName) \
   case proto::Update::MsgName: \
      MsgName(msg.GetExtension(proto::MsgName::extension)); \
      break;

   switch (msg.type()) {
      DISPATCH_MSG(BeginUpdate);
      DISPATCH_MSG(EndUpdate);
      DISPATCH_MSG(SetServerTick);
      DISPATCH_MSG(AllocObjects);
      DISPATCH_MSG(UpdateObject);
      DISPATCH_MSG(RemoveObjects);
      DISPATCH_MSG(UpdateDebugShapes);
      DISPATCH_MSG(DefineRemoteObject);
   default:
      ASSERT(false);
   }

   return true;
#if 0
   for (int i = 0; i < msg.replies_size(); i++) {
   const ::radiant::proto::Reply &reply = msg.replies(i);
   ReplyQueue::value_type &head = replyQueue_.front();
   if (head.first == reply.id()) {
      head.second(reply);
      replyQueue_.pop_front();
   }
#endif
}

void Client::BeginUpdate(const proto::BeginUpdate& msg)
{
   if (msg.sequence_number() != last_sequence_number_) {
      if (last_sequence_number_ > 0) {
         Reset();
      }
      last_sequence_number_ = msg.sequence_number();
   }
}


void Client::EndUpdate(const proto::EndUpdate& msg)
{

   for (om::EntityRef e : alloced_) {
      om::EntityPtr entity = e.lock();
      if (entity) {
         OnEntityAlloc(entity);
      }
   }
   alloced_.clear();

#if 0
   for (const auto& entry : _entitiesTraces) {
      JSONNode data;
      if (entry.second->Flush(data)) {
         RestAPI::SessionId sessionId = entry.first.first;
         dm::TraceId traceId = entry.first.second;

         JSONNode obj;
         obj.push_back(JSONNode("trace_id", traceId));
         data.set_name("data");
         obj.push_back(data);
         GetAPI().QueueEventFor(sessionId, "radiant.events.trace_fired", obj);
      }
   }
#endif

   // Only fire the remote store traces at sequence boundaries.  The
   // data isn't guaranteed to be in a consistent state between
   // boundaries.
   store_.FireTraces();
}

void Client::QueueEvent(std::string type, JSONNode payload)
{
   JSONNode e(JSON_NODE);
   e.push_back(JSONNode("type", type));
   e.push_back(JSONNode("when", Client::GetInstance().Now()));

   payload.set_name("data");
   e.push_back(payload);

   queued_events_.push_back(e);
}

void Client::SetServerTick(const proto::SetServerTick& msg)
{
   int now = msg.now();
   update_interpolation(now);
   Renderer::GetInstance().SetServerTick(now);
}

void Client::AllocObjects(const proto::AllocObjects& update)
{
   const auto& msg = update.objects();

   for (const proto::AllocObjects::Entry& entry : msg) {
      dm::ObjectId id = entry.object_id();

      // Some objects are created and registered by other objects, so
      // this 
      // LOG(WARNING) << "Allocating new client object " << id;
      ASSERT(!store_.FetchStaticObject(id));

      dm::ObjectPtr obj = store_.AllocSlaveObject(entry.object_type(), id); //om::AllocObject(entry.object_type(), store_);

      ASSERT(obj->GetObjectId() == id);
      objects_[id] = obj;
      if (obj->GetObjectType() == om::EntityObjectType) {
         LOG(INFO) << "adding entity " << id << " to alloc list.";
         alloced_.push_back(std::static_pointer_cast<om::Entity>(obj));
      }

      guards_ += obj->TraceObjectLifetime("client obj tracking",
                                          std::bind(&Client::OnDestroyed, this, id));
   }
}

void Client::UpdateObject(const proto::UpdateObject& update)
{
   const auto& msg = update.object();
   dm::ObjectId id = msg.object_id();

   //LOG(WARNING) << "Client updating object " << id << ".";

   dm::Object* obj = store_.FetchStaticObject(id);
   ASSERT(obj);
   obj->LoadObject(msg);

#if 0
   if (obj) {
      obj->LoadObject(msg);
   } else {
      dm::ObjectPtr obj = store_.AllocObject(msg);

      ASSERT(!objects_[id]);
      objects_[id] = obj;

      traces_ += obj->TraceObjectLifetime("client obj tracking",
                                          std::bind(&Client::OnDestroyed, this, id));
   }
#endif

   if (id == 1 && !rootObject_) {
      rootObject_ = store_.FetchObject<om::Entity>(id);
      Renderer::GetInstance().Initialize(rootObject_);
   }
}

void Client::RemoveObjects(const proto::RemoveObjects& update)
{
   const auto& msg = update.objects();

   for (int id : update.objects()) {
      auto i = objects_.find(id);

      if (selectedObject_ && id == selectedObject_->GetObjectId()) {
         SelectEntity(nullptr);
      }
      if (rootObject_ && id == rootObject_ ->GetObjectId()) {
         rootObject_  = nullptr;
      }
      Renderer::GetInstance().RemoveRenderObject(GetStore().GetStoreId(), id);

      ASSERT(i != objects_.end());

      // Not necessarily true... only true for entities.  Entities
      // explicitly hold references to their components!
      // ASSERT(i->second.use_count() == 1);
      if (i->second->GetObjectType() == om::EntityObjectType) {
         ASSERT(i->second.use_count() == 2);
      }

      if (i != objects_.end()) {
         objects_.erase(i);
         entities_.erase(id);
      }

   }
}

void Client::UpdateDebugShapes(const proto::UpdateDebugShapes& msg)
{
   Renderer::GetInstance().DecodeDebugShapes(msg.shapelist());
}

void Client::DefineRemoteObject(const proto::DefineRemoteObject& msg)
{
   std::string const &key = msg.uri();
   std::string const &value = msg.object_uri();

   if (value.empty()) { 
      auto i = serverRemoteObjects_.find(key);
      if (i != serverRemoteObjects_.end()) {
         serverRemoteObjects_.erase(i);
      }
   } else {
      serverRemoteObjects_[key] = value;
   }
}

void Client::RegisterReplyHandler(const proto::Command* cmd, ReplyFn fn)
{
   replyQueue_.push_back(std::make_pair(cmd->id(), fn));
}


void Client::process_messages()
{
   PROFILE_BLOCK();

   _io_service.poll();
   _io_service.reset();
   ProcessReadQueue();
}

void Client::ProcessReadQueue()
{
   if (recv_queue_) {
      recv_queue_->Process<proto::Update>(std::bind(&Client::ProcessMessage, this, std::placeholders::_1));
   }
}

void Client::update_interpolation(int time)
{
   PROFILE_BLOCK();

   if (_server_last_update_time) {
      _server_interval_duration = time - _server_last_update_time;
   }
   _server_last_update_time = time;

   // We want to interpolate from the current point to the new destiation over
   // the interval returned by the server.
   _client_interval_start = platform::get_current_time_in_ms();
}

void Client::activate_comamnd(int k)
{
}

void Client::OnMouseInput(const MouseEvent &mouse, bool &handled, bool &uninstall)
{
   bool hovering_on_brick = false;

   if (browser_->HasMouseFocus()) {
      // xxx: not quite right...
      return;
   }

   auto i = mouseEventPromises_.begin();
   while (!handled && i != mouseEventPromises_.end()) {
      auto promise = i->lock();
      if (promise && promise->IsActive()) {
         promise->OnMouseEvent(mouse, handled);
         i++;
      } else {
         i = mouseEventPromises_.erase(i);
      }
   }

   if (!handled) {
      if (mouse.up[0]) {
         LOG(WARNING) << "got mouse up... current command:" << currentCommand_;
      }
      if (rootObject_ && !currentCommand_ && mouse.up[0]) {
         LOG(WARNING) << "updating selection...";
         UpdateSelection(mouse);
      } else if (mouse.up[2]) {
         CenterMap(mouse);
      }
   }
}

void Client::CenterMap(const MouseEvent &mouse)
{
   om::Selection s;

   Renderer::GetInstance().QuerySceneRay(mouse.x, mouse.y, s);
   if (s.HasBlock()) {
      Renderer::GetInstance().PointCamera(math3d::point3(s.GetBlock()));
   }
}

void Client::UpdateSelection(const MouseEvent &mouse)
{
   om::Selection s;
   Renderer::GetInstance().QuerySceneRay(mouse.x, mouse.y, s);

   math3d::ray3 r = Renderer::GetInstance().GetCameraToViewportRay(mouse.x, mouse.y);
   om::EntityPtr stockpile = NULL;

   float minDistance = FLT_MAX;
   auto cb = [&minDistance, &stockpile] (om::EntityPtr obj, float d) {
#if 0
      auto s = obj->GetComponent<om::StockpileDesignation>();
      if (s && d < minDistance) {
         minDistance = d;
         stockpile = obj;
      }
#endif
   };
   octtree_->TraceRay(r, cb);
   if (stockpile) {
      LOG(WARNING) << "selecting stockpile";
      SelectEntity(stockpile);
   } else {
      if (s.HasEntities()) {
#if 0
         for (dm::ObjectId id : s.GetEntities()) {
            auto entity = GetEntity(id);
            if (entity && entity->GetComponent<om::Room>()) {
               LOG(WARNING) << "selecting room " << entity->GetObjectId();
               SelectEntity(entity);
               return;
            }
         }
#endif
         auto entity = GetEntity(s.GetEntities().front());
         if (entity->GetComponent<om::Terrain>()) {
            LOG(WARNING) << "clearing selection (clicked on terrain)";
            SelectEntity(nullptr);
         } else {
            LOG(WARNING) << "selecting " << entity->GetObjectId();
            SelectEntity(entity);
         }
      } else {
         LOG(WARNING) << "no entities!";
         SelectEntity(nullptr);
      }
   }
}


// xxx: move this crap into a separate class
void Client::OnKeyboardInput(const KeyboardEvent &e, bool &handled, bool &uninstall)
{
   if (e.down) {
      auto i = _commands.find(e.key);
      if (i != _commands.end()) {
         i->second();
         return;
      }
   }
}

void Client::SelectEntity(dm::ObjectId id)
{
   auto i = entities_.find(id);
   if (i != entities_.end()) {
      SelectEntity(i->second);
   }
}

void Client::SelectEntity(om::EntityPtr obj)
{
   if (selectedObject_ != obj) {
      auto renderEntity = Renderer::GetInstance().GetRenderObject(selectedObject_);
      if (renderEntity) {
         renderEntity->SetSelected(false);
      }

      selectedObject_ = obj;
      if (selectedObject_) {
         LOG(WARNING) << "Selected actor " << selectedObject_->GetObjectId();
      } else {
         LOG(WARNING) << "Cleared selected actor.";
      }
      renderEntity = Renderer::GetInstance().GetRenderObject(selectedObject_);
      if (renderEntity) {
         renderEntity->SetSelected(true);
      }

      JSONNode data(JSON_NODE);
      if (selectedObject_) {
         std::string uri = std::string("/object/") + stdutil::ToString(selectedObject_->GetObjectId());
         data.push_back(JSONNode("selected_entity", uri));
      }
      QueueEvent("selection_changed.radiant", data);
   }
}

void Client::PushServerRequest(proto::Request& msg, ServerReplyCb replyCb)
{
   int id = ++last_server_request_id_;
   msg.set_request_id(id);
   server_requests_[id] = replyCb;

   send_queue_->Push(msg);
}

void Client::EvalCommand(std::string cmd)
{
   auto reply = [=](tesseract::protocol::Update const& msg) {
      proto::ScriptCommandReply const& reply = msg.GetExtension(proto::ScriptCommandReply::extension);
      LOG(WARNING) << reply.result();
   };

   proto::Request msg;
   msg.set_type(proto::Request::ScriptCommandRequest);
   
   proto::ScriptCommandRequest* request = msg.MutableExtension(proto::ScriptCommandRequest::extension);
   request->set_cmd(cmd);

   PushServerRequest(msg, reply);
}

#if 0
vector<MaterialDescription> Client::GetAvailableMaterials(vector<string> tags)
{
   vector<MaterialDescription> result;
   ASSERT(false);
   return result;
#if 0
   map<string, int> resources;

   Terrain terrain = GetTerrain();

   List &items = *terrain.GetEntity()->Get<List >("items");
   for (auto obj : items) {
      Item item(obj);
      if (item.GetType() == "Item") { // xxx: make this Item::Prototype::Type instead of "Item"
         if (item.HasTags(tags)) {
            resources[item.GetIdentifier()]++;
         }
      }
   };

   world::Container &prototypes = *_proxy_manager->GetRootObject()->Get<Container>("prototypes");
   vector<MaterialDescription> result;

   for_each(resources.begin(), resources.end(), [&](const map<string, int>::value_type &i) {
      
      Item::Prototype item(prototypes.Get<Container>(i.first));
      ASSERT(item.IsValid());

      MaterialDescription md;
      md.name = item.GetName();
      md.identifier = i.first;
      md.count = i.second;
      result.push_back(md);
   });

   return result;
#endif
}
#endif

bool Client::ToggleBuildPlan()
{
   ShowBuildPlan(!showBuildOrders_);
   return true;
}

void Client::ShowBuildPlan(bool visible)
{
   if (showBuildOrders_  != visible) {
      showBuildOrders_ = visible;
      SetRenderPipelineInfo();
      traces_.FireTraces(ShowBuildOrdersTraces);
   }

#if 0
   auto id = rootObject_->GetBlueprints()->GetId();
   auto renderObject = Renderer::GetInstance().GetRenderObject(id);   

   bool showBuildPlan = renderObject ? visible : false;
   if (showBuildPlan != showBuildOrders_) {
      showBuildOrders_ = showBuildPlan;

      if (renderObject) {
         renderObject->SetVisible(showBuildOrders_);
      }

      // xxx - this is really expensive.  the correct way to do this is
      // to check in each render object after the render list has been
      // clipped by horde and have them update their state individually.
      // we have a ton of different view options (z-level view, rpg view,
      // underground view, etc).
      for (const auto& entry : allObjects_) {
         std::string type = entry.second->GetTypeName();
         if (type != "") {
            LOG(WARNING) << type;
         }
         if (type == "Citizen") {
            auto renderObj = Renderer::GetInstance().GetRenderObject(entry.first);
            renderObj->SetVisible(!showBuildOrders_);
         }
      }

      SetRenderPipelineInfo();
   }
#endif
}

#if 0
bool Client::CreateFloor(bool install)
{
   if (install) {
      auto cb = std::bind(&Client::AddToFloor, this, std::placeholders::_1);
      auto vrs = std::make_shared<VoxelRangeSelector>(GetTerrain(), cb);
      vrs->SetColor(math3d::color3(192, 192, 192));
      vrs->SetDimensions(2);
      selector_ = vrs;
      ShowBuildPlan(true);
   }
   return false;
}

bool Client::GrowWalls(bool install)
{

   // xxx: uninstall the input handler whenever the selection changes.
   auto onInput = [&](const MouseEvent &mouse, bool &handled, bool &uninstall) {
      if (mouse.up[0]) {
         uninstall = true;
         FinishCurrentCommand();
         return;
      } else {
         ObjectModel::Structure *building = GetSelected<ObjectModel::Structure>();

         if (building) {
            ObjectModel::Storey *storey = building->GetStorey(building->GetStoreyCount() - 1);
            if (storey) {
               math3d::ray3 ray = Renderer::GetInstance().GetCameraToViewportRay(mouse.x, mouse.y);
               int height = storey->ComputeHeightFromViewRay(ray);

               //auto *msg = _command_list.add_commands();
               //msg->set_type(::radiant::proto::Command::SetStoreyHeight);
               //auto *command = msg->MutableExtension(::radiant::proto::SetStoreyHeightCommand::extension);
               auto command = ADD_COMMAND_TO_QUEUE(SetStoreyHeight);
               command->set_storey(storey->GetId());
               command->set_height(height);
            }
         }
      }
      handled = true;
   };

   ObjectModel::Structure *building = GetSelected<ObjectModel::Structure>();
   if (building && install) {
      Renderer::GetInstance().SetMouseInputCallback(onInput);
      return false;
   }
   return true;
}

// This entire function is crappy... (see the static, etc.)
bool Client::PlaceFixture(bool install, dm::ObjectId fixtureId)
{
   static InputCallbackId eventHandlerId = 0;
   
   if (install) {
      ObjectModel::Structure* building = GetSelected<ObjectModel::Structure>();
      if (!building) {
         return true;
      }

      ObjectModel::Storey* storey = building->GetStorey(building->GetStoreyCount() - 1);

      // xxx: uninstall the input handler whenever the selection changes.
      auto onInput = [fixtureId, storey, this](const MouseEvent &mouse, bool &handled, bool &uninstall) {
         if (mouse.up[0]) {
            FinishCurrentCommand();
            return;
         } else {
            math3d::ray3 ray = Renderer::GetInstance().GetCameraToViewportRay(mouse.x, mouse.y);

            //auto *msg = _command_list.add_commands();
            //msg->set_type(::radiant::proto::Command::SetStoreyHeight);
            //auto *command = msg->MutableExtension(::radiant::proto::SetStoreyHeightCommand::extension);
            auto command = ADD_COMMAND_TO_QUEUE(SetFixturePosition);
            command->set_fixture(fixtureId);
            command->set_storey(storey->GetId());
            ray.encode(command->mutable_ray());
         }
         handled = true;
      };
      eventHandlerId = Renderer::GetInstance().SetMouseInputCallback(onInput);
      return false;
   }
   if (eventHandlerId) {
      Renderer::GetInstance().RemoveInputEventHandler(eventHandlerId);
      eventHandlerId = 0;
   }
   return true;
}

bool Client::AddFixture(bool install, std::string fixtureName)
{
   ObjectModel::Structure* building = GetSelected<ObjectModel::Structure>();
   if (!building) {
      return false;
   }
   ObjectModel::Storey* storey = building->GetStorey(building->GetStoreyCount() - 1);
   if (!storey) {
      return false;
   }

   auto dispatch = [&](const proto::Reply& reply) {
      if (reply.type() == proto::Reply::CreateFixture) {
         auto extension = reply.GetExtension(proto::CreateFixtureReply::extension);
         RunCommand(std::bind(&Client::PlaceFixture, this, std::placeholders::_1, extension.fixture()));
      }
   };

   auto command = ADD_COMMAND_TO_QUEUE_EX(CreateFixture, dispatch);
   command->set_fixture_name(fixtureName);
   return true;
}

bool Client::CapStorey(bool install)
{
   ObjectModel::Structure* building = GetSelected<ObjectModel::Structure>();

   if (building) {
      auto command = ADD_COMMAND_TO_QUEUE(CapStorey);
      command->set_building(building->GetId());
   }

   return true;
}

bool Client::FinishStructure(bool install)
{
   ObjectModel::Structure* building = GetSelected<ObjectModel::Structure>();

   if (building) {
      auto command = ADD_COMMAND_TO_QUEUE(FinishStructure);
      command->set_building(building->GetId());
   }

   return true;
}


void Client::AddToFloor(const om::Selection &selection)
{
   auto command = ADD_COMMAND_TO_QUEUE(BuildFloor);

   selection.bounds.encode(command->mutable_bounds());
}

void Client::AssignWorkerToBuilding(vector<om::Selection> args)
{
   auto command = ADD_COMMAND_TO_QUEUE(AssignWorkerToBuilding);

   command->set_building(args[0].actors[0]);
   command->set_worker(args[1].actors[0]);
}
#endif

void Client::OnObjectLoad(std::vector<dm::ObjectId> objects)
{
#if 1
   ASSERT(false);
#else
   bool initRootObject = false;

   for (auto id : objects) {
      auto object = ObjectModel::Foo::RestoreObject(_proxy_manager->GetEntity(id));
      if (object) {
         ASSERT(allObjects_[id] == NULL);
         allObjects_[id] = object;

         if (!rootObject_) {
            rootObject_ = object->GetInterface<ObjectModel::RootObject>();
            initRootObject = true;
         }
      }
   }
   if (initRootObject) {
      octtree_->Initialize(rootObject_);
      Renderer::GetInstance().Initialize(rootObject_);
   }
#endif
}

om::EntityPtr Client::GetEntity(dm::ObjectId id)
{
   auto obj = objects_[id];
   return (obj && obj->GetObjectType() == om::Entity::DmType) ? std::static_pointer_cast<om::Entity>(obj) : nullptr;
}

void Client::InstallCursor()
{
   if (browser_) {
      HCURSOR cursor;
      if (browser_->HasMouseFocus()) {
         cursor = uiCursor_;
      } else {
         cursor = NULL;
         if (!currentToolName_.empty()) {
            auto i = cursors_.find(currentToolName_);
            if (i != cursors_.end()) {
               cursor = i->second.get();
            }
         }
         if (!cursor && currentCommandCursor_) {
            cursor = currentCommandCursor_;
         }
         if (!cursor && !hilightedObjects_.empty()) {
            cursor = cursors_["hover"].get();
         }
         if (!cursor) {
            cursor = cursors_["default"].get();
         }
      }
      if (cursor != currentCursor_) {
         SetClassLong(hwnd_, GCL_HCURSOR, static_cast<LONG>(reinterpret_cast<LONG_PTR>(cursor)));
         SetCursor(cursor);
         currentCursor_ = cursor;
      }
   }
}

void Client::SetUICursor(HCURSOR cursor)
{
   if (uiCursor_) {
      DestroyCursor(uiCursor_);
   }
   if (cursor) {
      uiCursor_ = CopyCursor(cursor);
   } else {
      uiCursor_ = NULL;
   }
}

#if 0
void Client::ExecuteCommands()
{  
   std::vector<PendingCommandPtr> commands = GetAPI().GetPendingCommands();
   for (PendingCommandPtr cmd : commands) {

      currentCommandCursor_ = NULL;
      currentCommand_ = nullptr;
      currentTool_ = nullptr;
      currentToolName_ = "";

      CommandPtr command;

      JSONNode const& json = cmd->GetJson();
      std::string type = json["command"].as_string();
      if (type == "execute_action") {
         command = std::make_shared<ExecuteAction>(cmd);
      } else if (type == "select_tool") {
         SelectToolCommand(cmd);
      } else if (type == "select_entity") {
         SelectEntityCommand(cmd);
      } else if (type == "trace_entities") {
         TraceEntities(cmd);
      } else if (type == "trace_entity") {
         TraceEntity(cmd);
      } else if (type == "fetch_json_data") {
         FetchJsonData(cmd);
      } else if (type == "set_view_mode") {
         SetViewModeCommand(cmd);
      } else {
         cmd->Error("Unknown type: " + type);
      }
      if (command) {
         currentCommand_ = command;
         (*command)();
      }
   }
}
#endif

void Client::SetRenderPipelineInfo()
{
   std::string pipeline = "pipelines/deferred_pipeline_static.xml";
   if (showBuildOrders_) {
      //pipeline = "pipelines/blueprint.deferred.pipeline.xml";
   }
   Renderer::GetInstance().SetCurrentPipeline(pipeline);
}

bool Client::RotateViewMode(bool install)
{
   auto& r = Renderer::GetInstance();
   
   ViewMode mode = r.GetViewMode();
   mode = (ViewMode)((mode+ 1) % MaxViewModes);

   r.SetViewMode(mode);
   return true;
}

void Client::OnDestroyed(dm::ObjectId id)
{
   LOG(INFO) << "object "  << id << " has been destroyed.";
}

void Client::SelectGroundArea(Selector::SelectionFn cb)
{
   auto callback = [=](const om::Selection &s) {
      cb(s);
      selector_ = nullptr;
   };

   auto vrs = std::make_shared<VoxelRangeSelector>(GetTerrain(), cb);
   vrs->SetColor(math3d::color3(192, 192, 192));
   vrs->SetDimensions(2);
   vrs->Activate();
   if (selector_) {
      selector_->Deactivate();
   }
   selector_ = vrs;
}


dm::Guard Client::TraceShowBuildOrders(std::function<void()> cb)
{
   return traces_.AddTrace(ShowBuildOrdersTraces, cb);
}

void Client::EnumEntities(std::function<void(om::EntityPtr)> cb)
{
   for (const auto& entry : entities_) {
      cb(entry.second);
   }
}

#if 0

void Client::TraceEntities(PendingCommandPtr cmd)
{
#if 0
   const auto& args = cmd->GetArgs();

   CHECK_TYPE(cmd, args, "filters", JSON_ARRAY);
   CHECK_TYPE(cmd, args, "collectors", JSON_ARRAY);

   int traceId = nextTraceId_++;
   
   auto trace = std::make_shared<EntitiesTrace>(args);
   _entitiesTraces[std::make_pair(cmd->GetSession(), traceId)] = trace;

   JSONNode result, data(JSON_ARRAY);
   trace->Flush(data);
   data.set_name("data");

   result.push_back(JSONNode("trace_id", traceId));
   result.push_back(data);
   cmd->Complete(result);
#endif
   ASSERT(false);
}

void Client::FetchJsonData(PendingCommandPtr cmd)
{
#if 0
   const auto& args = cmd->GetArgs();

   CHECK_TYPE(cmd, args, "entries", JSON_ARRAY);
   
   JSONNode result;
   for (const auto& node : args["entries"]) {
      if (node.type() == JSON_STRING) {
         std::string name = node.as_string();
         JSONNode const& data = resources::ResourceManager2::GetInstance().LookupJson(name);
         result.push_back(data);
      }
   }
   cmd->Complete(result);
#endif
   ASSERT(false);
}

void Client::TraceEntity(PendingCommandPtr cmd)
{
#if 0
   const auto& args = cmd->GetArgs();

   CHECK_TYPE(cmd, args, "filters", JSON_ARRAY);
   CHECK_TYPE(cmd, args, "collectors", JSON_ARRAY);
   CHECK_TYPE(cmd, args, "entity_id", JSON_NUMBER);

   om::EntityPtr entity = GetEntity(args["entity_id"].as_int());
   if (!entity) {
      cmd->Error("entity does not exist");
      return;
   }
   int traceId = nextTraceId_++;
   
   auto trace = std::make_shared<EntityTrace>(entity, args);
   _entitiesTraces[std::make_pair(cmd->GetSession(), traceId)] = trace;

   JSONNode result, data(JSON_ARRAY);
   trace->Flush(data);
   data.set_name("data");

   result.push_back(JSONNode("trace_id", traceId));
   result.push_back(data);
   cmd->Complete(result);
#endif
   ASSERT(false);
}

void Client::SetViewModeCommand(PendingCommandPtr cmd)
{
#if 0
   const auto& args = cmd->GetArgs();

   CHECK_TYPE(cmd, args, "mode", JSON_STRING);

   std::string mode = args["mode"].as_string();
   if (mode == "build") {
      ShowBuildPlan(true);
   } else {
      ShowBuildPlan(false);
   }
   cmd->Complete(JSONNode());
#endif
   ASSERT(false);
}

void Client::SelectEntityCommand(PendingCommandPtr cmd)
{
   const auto& json = cmd->GetJson();

   CHECK_TYPE(cmd, json, "entity_id", JSON_NUMBER);

   Client::GetInstance().SelectEntity(json["entity_id"].as_int());
   cmd->CompleteSuccessObj(JSONNode());
}

void Client::SelectToolCommand(PendingCommandPtr cmd)
{
   const auto& args = cmd->GetJson();

   LOG(WARNING) << args.write();
   CHECK_TYPE(cmd, args, "tool", JSON_STRING);

   std::shared_ptr<Command>  tool;
   std::string toolName = args["tool"].as_string();
   if (toolName == "none") {
      cmd->CompleteSuccessObj(JSONNode());
   } else if (toolName == "create_walls") {
      tool = std::make_shared<CreateRoom>(cmd);
   } else if (toolName == "create_door") {
      tool = std::make_shared<CreatePortal>(cmd);
   } else {
      cmd->Error(std::string("No toolName named \"") + toolName + std::string("\""));
      return;
   }

   currentTool_ = tool;
   currentToolName_ = tool ? toolName : "";

   currentCommand_ = nullptr;
   currentCommandCursor_ = NULL;

   if (currentTool_)  {
      (*currentTool_)();
   }
}

int Client::CreatePendingCommandResponse()
{
   int id = nextResponseHandlerId_++;
   responseHandlers_[id] = [=](const proto::DoActionReply* msg) {
      JSONNode result;
      result.push_back(JSONNode("pending_command_id", id));

      if (msg) {
         result.push_back(JSONNode("result", "success"));

         JSONNode data = libjson::parse(msg->json());
         data.set_name("response");
         result.push_back(data);
         LOG(WARNING) << data.write();
         LOG(WARNING) << result.write();
      } else {
         ASSERT(false);
      }
      QueueEvent("radiant.events.pending_command_completed", result);
   };
   return id;
}

int Client::CreateCommandResponse(CommandResponseFn fn)
{
   int id = nextResponseHandlerId_++;
   responseHandlers_[id] = fn;
   return id;
}

#endif

void Client::HilightMouseover()
{
   om::Selection selection;
   auto &renderer = Renderer::GetInstance();
   csg::Point2 pt = renderer.GetMousePosition();

   renderer.QuerySceneRay(pt.x, pt.y, selection);

   for (const auto &e: hilightedObjects_) {
      auto entity = e.lock();
      if (entity && entity != selectedObject_) {
         auto renderObject = renderer.GetRenderObject(entity);
         if (renderObject) {
            renderObject->SetSelected(false);
         }
      }
   }
   hilightedObjects_.clear();
   if (selection.HasEntities()) {
      auto entity = GetEntity(selection.GetEntities().front());
      if (entity && entity != rootObject_) {
         auto renderObject = renderer.GetRenderObject(entity);
         if (renderObject) {
            renderObject->SetSelected(true);
         }
         hilightedObjects_.push_back(entity);
      }
   }
}

void Client::LoadCursors()
{
   fs::recursive_directory_iterator i("cursors"), end;
   while (i != end) {
      fs::path path(*i++);
      if (fs::is_regular_file(path) && path.extension() == ".cur") {
         HCURSOR cursor = (HCURSOR)LoadImage(GetModuleHandle(NULL), path.c_str(), IMAGE_CURSOR, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
         if (cursor) {
            std::string name = fs::basename(path);
            cursors_[name].reset(cursor);
         }
      }
   }
}

void Client::SetCommandCursor(std::string name)
{
   auto i = cursors_.find(name);
   if (i != cursors_.end()) {
      currentCommandCursor_ = i->second.get();
   } else {
      currentCommandCursor_ = NULL;
   }
}

void Client::OnEntityAlloc(om::EntityPtr entity)
{
   dm::ObjectId id = entity->GetObjectId();

   entities_[id] = entity;
   if (id == 1) {
      octtree_->SetRootEntity(entity);
   }
#if 0
   LOG(INFO) << "tracing components of entity " << id;

   const auto& components = entity->GetComponents();
   guards_ += components.TraceMapChanges("client component checking",
                                          std::bind(&Client::ComponentAdded, this, om::EntityRef(entity), std::placeholders::_1, std::placeholders::_2),
                                          nullptr);
   for (const auto& entry : components) {
      ComponentAdded(entity, entry.first, entry.second);
   }
#endif
}

void Client::ComponentAdded(om::EntityRef e, dm::ObjectType type, std::shared_ptr<dm::Object> component)
{
}

static void HandleFileRequest(std::string const& path, std::shared_ptr<net::IResponse> response)
{
   static const struct {
      char *extension;
      char *mimeType;
   } mimeTypes_[] = {
      { "htm",  "text/html" },
      { "html", "text/html" },
      {  "css",  "text/css" },
      { "less",  "text/css" },
      { "js",   "application/x-javascript" },
      { "json", "application/json" },
      { "txt",  "text/plain" },
      { "jpg",  "image/jpeg" },
      { "png",  "image/png" },
      { "gif",  "image/gif" },
      { "woff", "application/font-woff" },
      { "cur",  "image/vnd.microsoft.icon" },
   };
   std::ifstream infile;
   auto const& rm = resources::ResourceManager2::GetInstance();

   std::string data;
   try {
      if (boost::ends_with(path, ".json")) {
         JSONNode const& node = rm.LookupJson(path);
         data = node.write();
      } else {
         rm.OpenResource(path, infile);
         data = io::read_contents(infile);
      }
   } catch (resources::Exception const& e) {
      LOG(WARNING) << "error code 404: " << e.what();
      response->SetResponse(404);
      return;
   }


   // Determine the file extension.
   std::string mimeType;
   std::size_t last_dot_pos = path.find_last_of(".");
   if (last_dot_pos != std::string::npos) {
      std::string extension = path.substr(last_dot_pos + 1);
      for (auto &entry : mimeTypes_) {
         if (extension == entry.extension) {
            mimeType = entry.mimeType;
            break;
         }
      }
   }
   ASSERT(!mimeType.empty());
   response->SetResponse(200, data, mimeType);
}

void Client::HandleClientRouteRequest(luabind::object ctor, JSONNode const& query, std::string const& postdata, std::shared_ptr<net::IResponse> response)
{
   // convert the json query data to a lua table...
   try {
      using namespace luabind;
      lua_State* L = scriptHost_->GetCallbackState();
      object queryObj = scriptHost_->JsonToLua(query);
      object coder = globals(L)["radiant"]["json"];

      object obj = scriptHost_->CallFunction<object>(ctor);
      object fn = obj["handle_request"];
      auto i = query.find("fn");
      if (i != query.end() && i->type() == JSON_STRING) {
         std::string fname = i->as_string();
         if (!fname.empty())  {
            fn = obj[fname];
         }
      }
      object result = scriptHost_->CallFunction<object>(fn, obj, queryObj, postdata);
      std::string json = scriptHost_->CallFunction<std::string>(coder["encode"], result);

      response->SetResponse(200, json, "application/json");
   } catch (std::exception &e) {
      JSONNode result;
      result.push_back(JSONNode("error", e.what()));
      response->SetResponse(500, result.write(), "application/json");
   }
}

void Client::HandlePostRequest(std::string const& path, JSONNode const& query, std::string const& postdata, std::shared_ptr<net::IResponse> response)
{
   auto replyCb = [=](tesseract::protocol::Update const& msg) {
      proto::PostCommandReply const& reply = msg.GetExtension(proto::PostCommandReply::extension);
      response->SetResponse(reply.status_code(), reply.content(), reply.mime_type());
   };
   proto::Request msg;
   msg.set_type(proto::Request::PostCommandRequest);
   
   proto::PostCommandRequest* request = msg.MutableExtension(proto::PostCommandRequest::extension);
   request->set_path(path);
   request->set_data(postdata);
   if (!query.empty()) {
      request->set_query(query.write());
   }
   PushServerRequest(msg, replyCb);
}

void Client::GetRemoteObject(std::string const& uri, JSONNode const& query, std::shared_ptr<net::IResponse> response)
{
   auto reply = [=](tesseract::protocol::Update const& msg) {
      proto::FetchObjectReply const& r = msg.GetExtension(proto::FetchObjectReply::extension);
      response->SetResponse(r.status_code(), r.content(), r.mime_type());
   };

   proto::Request msg;
   msg.set_type(proto::Request::FetchObjectRequest);
   
   proto::FetchObjectRequest* request = msg.MutableExtension(proto::FetchObjectRequest::extension);
   request->set_uri(uri);
         
   PushServerRequest(msg, reply);
}

void Client::GetEvents(JSONNode const& query, std::shared_ptr<net::IResponse> response)
{
   if (get_events_request_) {
      get_events_request_->SetResponse(504);
   }
   get_events_request_ = response;
   FlushEvents();
}

void Client::GetModules(JSONNode const& query, std::shared_ptr<net::IResponse> response)
{
   JSONNode result;
   auto& rm = resources::ResourceManager2::GetInstance();
   for (std::string const& modname : rm.GetModuleNames()) {
      JSONNode manifest;
      try {
         manifest = rm.LookupManifest(modname);
      } catch (std::exception const& e) {
         // Just use an empty manifest...f
      }
      manifest.set_name(modname);
      result.push_back(manifest);
   }
   response->SetResponse(200, result.write(), "application/json");
}

void Client::TraceUri(JSONNode const& query, std::shared_ptr<net::IResponse> response)
{
   json::ConstJsonObject args(query);

   if (args["create"].as_bool()) {
      std::string uri = args["uri"].as_string();

      auto i = serverRemoteObjects_.find(uri);
      if (i != serverRemoteObjects_.end()) {
         uri = i->second;
      }

      if (boost::starts_with(uri, "/object")) {
         TraceObjectUri(uri, response);
      } else {
         TraceFileUri(uri, response);
      }
   } else {
      dm::TraceId traceId = args["trace_id"].as_integer();
      uriTraces_.erase(traceId);
   }
}

void Client::TraceObjectUri(std::string const& uri, std::shared_ptr<net::IResponse> response)
{
   dm::ObjectPtr obj = om::ObjectFormatter("/object/").GetObject(GetStore(), uri);
   if (obj) {
      int traceId = nextTraceId_++;
      std::weak_ptr<dm::Object> o = obj;

      auto createResponse = [traceId, uri, o]() -> JSONNode {
         JSONNode node;
         node.push_back(JSONNode("trace_id", traceId));
         node.push_back(JSONNode("uri",      uri));
         auto obj = o.lock();
         if (obj) {
            JSONNode data = om::ObjectFormatter("/object/").ObjectToJson(obj);
            data.set_name("data");
            node.push_back(data);
         }
         return node;
      };

      auto objectChanged = [this, createResponse]() {
         QueueEvent("radiant.trace_fired", createResponse());
      };

      uriTraces_[traceId] = obj->TraceObjectChanges("tracing uri", objectChanged);
      JSONNode res = createResponse();
      response->SetResponse(200, res.write(), "application/json");
      return;
   }
   response->SetResponse(404, "", "");
}

void Client::TraceFileUri(std::string const& uri, std::shared_ptr<net::IResponse> response)
{
   if (boost::ends_with(uri, ".json")) {
      auto const& rm = resources::ResourceManager2::GetInstance();
      JSONNode data = rm.LookupJson(uri);

      JSONNode node;
      int traceId = nextTraceId_++;
      node.push_back(JSONNode("trace_id", traceId));
      node.push_back(JSONNode("uri",      uri));
      data.set_name("data");
      node.push_back(data);
      response->SetResponse(200, node.write(), "application/json");
      return;
   }
   LOG(WARNING) << "attempting to trace non-json uri " << uri << " NOT IMPLEMENTED!!! Returning 404";
   response->SetResponse(404, "", "");
}

void Client::FlushEvents()
{
   if (get_events_request_ && !queued_events_.empty()) {
      JSONNode events(JSON_ARRAY);
      for (const auto& e : queued_events_) {
         events.push_back(e);
      }
      queued_events_.clear();
      get_events_request_->SetResponse(200, events.write(), "application/json");
      get_events_request_ = nullptr;
   }
}

void Client::BrowserRequestHandler(std::string const& path, JSONNode const& query, std::string const& postdata, std::shared_ptr<net::IResponse> response)
{
   auto i = clientRoutes_.find(path);
   if (i != clientRoutes_.end()) {
      HandleClientRouteRequest(i->second, query, postdata, response);
      return;
   }

   if (postdata.empty()) {
      if (boost::starts_with(path, "/api/trace")) {
         TraceUri(query, response);
      } else if (boost::starts_with(path, "/api/get_modules")) {
         GetModules(query, response);
      } else if (boost::starts_with(path, "/object")) {
         GetRemoteObject(path, query, response);
      } else {
         HandleFileRequest(path, response);
      }
   } else {
      HandlePostRequest(path, query, postdata, response);
   }
}
void Client::SetScriptHost(lua::ScriptHost* scriptHost)
{
   scriptHost_ = scriptHost;
}

om::EntityPtr Client::CreateAuthoringEntity(std::string const& path)
{
   om::EntityPtr entity = om::Stonehearth::CreateEntityLegacyDIEDIEDIE(authoringStore_, path);
   authoredEntities_[entity->GetObjectId()] = entity;
   return entity;
}

void Client::DestroyAuthoringEntity(dm::ObjectId id)
{
   auto i = authoredEntities_.find(id);
   if (i != authoredEntities_.end()) {
      authoredEntities_.erase(i);
   }
}

MouseEventPromisePtr Client::TraceMouseEvents()
{
   MouseEventPromisePtr result = std::make_shared<MouseEventPromise>();
   mouseEventPromises_.push_back(result);
   return result;
}

void client::RegisterLuaTypes(lua_State* L)
{
   LuaRenderer::RegisterType(L);
   LuaRenderEntity::RegisterType(L);
   Client::RegisterLuaTypes(L);
}

MouseEventPromisePtr MouseEventPromise_OnMouseEvent(MouseEventPromisePtr me, luabind::object cb)
{
   lua::ScriptHost* host = Client::GetInstance().GetScriptHost();

   using namespace luabind;
   me->TraceMouseEvent([=](const MouseEvent &e) -> bool {
      object result = host->CallFunction<object>(cb, e);
      return type(result) == LUA_TBOOLEAN && object_cast<bool>(result);
   });
   return me;
}

void MouseEventPromise_Destroy(MouseEventPromisePtr me)
{
   me->Uninstall();
}

om::EntityRef Client_CreateAuthoringEntity(Client& client, std::string const& path)
{
   return client.CreateAuthoringEntity(path);
}


static luabind::object
Client_QueryScene(lua_State* L, Client const&, int x, int y)
{
   om::Selection s;
   Renderer::GetInstance().QuerySceneRay(x, y, s);

   using namespace luabind;
   object result = newtable(L);
   if (s.HasBlock()) {
      result["location"] = object(L, s.GetBlock());
   }
   return result;
}

std::weak_ptr<RenderEntity> Client_CreateRenderEntity(Client& c, H3DNode parent, luabind::object arg)
{
   om::EntityPtr entity;
   std::weak_ptr<RenderEntity> result;

   if (luabind::type(arg) == LUA_TSTRING) {
      // arg is a path to an object (e.g. /objects/3).  If this leads to a Entity, we're all good
      std::string path = luabind::object_cast<std::string>(arg);
      dm::Store& store = Client::GetInstance().GetStore();
      dm::ObjectPtr obj =  om::ObjectFormatter("objects").GetObject(store, path);
      if (obj && obj->GetObjectType() == om::Entity::DmType) {
         entity = std::static_pointer_cast<om::Entity>(obj);
      }
   } else {
      try {
         entity = luabind::object_cast<om::EntityRef>(arg).lock();
      } catch (luabind::cast_failed&) {
      }
   }
   if (entity) {
      result = Renderer::GetInstance().CreateRenderObject(parent, entity);
   }
   return result;
}


static bool MouseEvent_GetUp(MouseEvent const& me, int button)
{
   button--;
   if (button >= 0 && button < sizeof(me.up)) {
      return me.up[button];
   }
   return false;
}

DeferredPtr Client_Call(lua_State* L, Client& c, std::string const& uri, std::string fn, luabind::object postdataObject)
{
   DeferredPtr response = std::make_shared<Deferred>();
   try {
      using namespace luabind;

      JSONNode query;
      query.push_back(JSONNode("fn", fn));

      lua::ScriptHost* scriptHost = Client::GetInstance().GetScriptHost();
      lua_State* L = scriptHost->GetCallbackState();
      object coder = globals(L)["radiant"]["json"];
      std::string postdata = scriptHost->CallFunction<std::string>(coder["encode"], postdataObject);

      c.BrowserRequestHandler(uri, query, postdata, response);
   } catch (std::exception const& e) {
      JSONNode result;
      result.push_back(JSONNode("error", e.what()));
      response->SetResponse(500, result.write(), "application/json");
   }
   return response;
}

DeferredPtr Deferred_Always(DeferredPtr d, luabind::object cb)
{
   if (d) {
      lua::ScriptHost* scriptHost = Client::GetInstance().GetScriptHost();
      lua_State* L = scriptHost->GetCallbackState();
      luabind::object cbObj(L, cb);

      d->Always([=]() {
         luabind::call_function<void>(cbObj);
      });
   }
   return d;
}


void Client::RegisterLuaTypes(lua_State* L)
{
   using namespace luabind;
   module(L) [
      lua::RegisterType<Client>()
         .def("create_authoring_entity",  &Client_CreateAuthoringEntity)
         .def("destroy_authoring_entity", &Client::DestroyAuthoringEntity)
         .def("create_render_entity",     &Client_CreateRenderEntity)
         .def("trace_mouse",              &Client::TraceMouseEvents)
         .def("query_scene",              &Client_QueryScene)
         .def("call",                     &Client_Call)
      ,
      lua::RegisterTypePtr<MouseEventPromise>()
         .def("on_mouse_event",     &MouseEventPromise_OnMouseEvent)
         .def("destroy",            &MouseEventPromise_Destroy)
      ,
      lua::RegisterType<MouseEvent>()
         .def_readonly("x",       &MouseEvent::x)
         .def_readonly("y",       &MouseEvent::y)
         .def_readonly("wheel",   &MouseEvent::wheel)
         .def("up",               &MouseEvent_GetUp)
      ,
      lua::RegisterTypePtr<Deferred>()
         .def("always",           &Deferred_Always)
   ];
}
