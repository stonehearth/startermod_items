#include "pch.h"
#include <regex>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include "core/config.h"
#include "radiant_exceptions.h"
#include "xz_region_selector.h"
#include "om/entity.h"
#include "om/components/terrain.h"
#include "om/error_browser/error_browser.h"
#include "om/selection.h"
#include "om/om_alloc.h"
#include "csg/lua/lua_csg.h"
#include "om/lua/lua_om.h"
#include "om/json_store.h"
#include "om/data_binding.h"
#include "platform/utils.h"
#include "resources/manifest.h"
#include "resources/res_manager.h"
#include "lua/radiant_lua.h"
#include "lua/register.h"
#include "lua/script_host.h"
#include "om/stonehearth.h"
#include "om/object_formatter/object_formatter.h"
#include "horde3d/Source/Horde3DEngine/egModules.h"
#include "horde3d/Source/Horde3DEngine/egCom.h"
#include "lib/rpc/trace.h"
#include "lib/rpc/untrace.h"
#include "lib/rpc/function.h"
#include "lib/rpc/core_reactor.h"
#include "lib/rpc/http_reactor.h"
#include "lib/rpc/reactor_deferred.h"
#include "lib/rpc/protobuf_router.h"
#include "lib/rpc/lua_module_router.h"
#include "lib/rpc/lua_object_router.h"
#include "lib/rpc/trace_object_router.h"
#include "lib/rpc/http_deferred.h" // xxx: does not belong in rpc!
#include "lua/client/open.h"
#include "lua/res/open.h"
#include "lua/rpc/open.h"
#include "lua/om/open.h"
#include "lua/voxel/open.h"
#include "client/renderer/render_entity.h"
#include "lib/perfmon/perfmon.h"
#include "glfw3.h"

//  #include "GFx/AS3/AS3_Global.h"
#include "client.h"
#include "renderer/renderer.h"
#include "libjson.h"

using namespace ::radiant;
using namespace ::radiant::client;
namespace fs = boost::filesystem;
namespace po = boost::program_options;
namespace proto = ::radiant::tesseract::protocol;

static const std::regex call_path_regex__("/r/call/?");

DEFINE_SINGLETON(Client);

Client::Client() :
   _tcp_socket(_io_service),
   _server_last_update_time(0),
   _server_interval_duration(1),
   selectedObject_(NULL),
   last_sequence_number_(-1),
   rootObject_(NULL),
   store_(2, "game"),
   authoringStore_(3, "tmp"),
   nextTraceId_(1),
   uiCursor_(NULL),
   currentCursor_(NULL),
   last_server_request_id_(0),
   next_input_id_(1),
   mouse_x_(0),
   mouse_y_(0),
   perf_hud_shown_(false)
{
   om::RegisterObjectTypes(store_);
   om::RegisterObjectTypes(authoringStore_);

   std::vector<std::pair<dm::ObjectId, dm::ObjectType>>  allocated_;
   scriptHost_.reset(new lua::ScriptHost());

   error_browser_ = authoringStore_.AllocObject<om::ErrorBrowser>();
   

   // Reactors...
   core_reactor_ = std::make_shared<rpc::CoreReactor>();
   http_reactor_ = std::make_shared<rpc::HttpReactor>(*core_reactor_);

   // Routers...
   core_reactor_->AddRouter(std::make_shared<rpc::LuaModuleRouter>(scriptHost_.get(), "client"));
   core_reactor_->AddRouter(std::make_shared<rpc::LuaObjectRouter>(scriptHost_.get(), GetAuthoringStore()));
   core_reactor_->AddRouter(std::make_shared<rpc::TraceObjectRouter>(GetStore()));
   core_reactor_->AddRouter(std::make_shared<rpc::TraceObjectRouter>(GetAuthoringStore()));

   // protobuf router should be last!
   protobuf_router_ = std::make_shared<rpc::ProtobufRouter>([this](proto::PostCommandRequest const& request) {
      proto::Request msg;
      msg.set_type(proto::Request::PostCommandRequest);   
      proto::PostCommandRequest* r = msg.MutableExtension(proto::PostCommandRequest::extension);
      *r = request;

      send_queue_->Push(msg);
   });
   core_reactor_->AddRouter(protobuf_router_);

   // core functionality exposed by the client...
   core_reactor_->AddRoute("radiant:get_modules", [this](rpc::Function const& f) {
      return GetModules(f);
   });
   core_reactor_->AddRoute("radiant:install_trace", [this](rpc::Function const& f) {
      json::Node args(f.args);
      std::string uri = args.get<std::string>(0, "");
      return http_reactor_->InstallTrace(rpc::Trace(f.caller, f.call_id, uri));
   });
   core_reactor_->AddRoute("radiant:remove_trace", [this](rpc::Function const& f) {
      return core_reactor_->RemoveTrace(rpc::UnTrace(f.caller, f.call_id));
   });
}

Client::~Client()
{
   octtree_.release();
}

void Client::GetConfigOptions()
{
}


extern bool realtime;
void Client::run()
{
   hover_cursor_ = LoadCursor("stonehearth:cursors:hover");
   default_cursor_ = LoadCursor("stonehearth:cursors:default");

   octtree_ = std::unique_ptr<phys::OctTree>(new phys::OctTree());
      
   Renderer& renderer = Renderer::GetInstance();
   //renderer.SetCurrentPipeline("pipelines/deferred_lighting.xml");
   //renderer.SetCurrentPipeline("pipelines/forward.pipeline.xml");

   Horde3D::Modules::log().SetNotifyErrorCb([=](om::ErrorBrowser::Record const& r) {
      error_browser_->AddRecord(r);
   });

   HWND hwnd = renderer.GetWindowHandle();
   //defaultCursor_ = (HCURSOR)GetClassLong(hwnd_, GCL_HCURSOR);

   renderer.SetInputHandler([=](Input const& input) {
      OnInput(input);
   });

   namespace po = boost::program_options;
   auto vm = core::Config::GetInstance().GetVarMap();
   std::string loader = vm["game.mod"].as<std::string>();
   json::Node manifest(res::ResourceManager2::GetInstance().LookupManifest(loader));
   std::string docroot = "http://radiant/" + manifest.get<std::string>("loader.ui.homepage");

   // seriously???
   std::string game_script = vm["game.script"].as<std::string>();
   if (game_script != "stonehearth/start_game.lua") {
      docroot += "?skip_title=true";
   }

   int screen_width = renderer.GetWidth();
   int screen_height = renderer.GetHeight();
   browser_.reset(chromium::CreateBrowser(hwnd, docroot, screen_width, screen_height, 1338));
   browser_->SetCursorChangeCb([=](HCURSOR cursor) {
      if (uiCursor_) {
         DestroyCursor(uiCursor_);
      }
      if (cursor) {
         uiCursor_ = CopyCursor(cursor);
      } else {
         uiCursor_ = NULL;
      }
   });
  

   browser_->SetRequestHandler([=](std::string const& uri, JSONNode const& query, std::string const& postdata, rpc::HttpDeferredPtr response) {
      BrowserRequestHandler(uri, query, postdata, response);
   });

   int ui_width, ui_height;
   browser_->GetBrowserSize(ui_width, ui_height);
   renderer.SetUITextureSize(ui_width, ui_height);

   guards_ += renderer.OnScreenResize([this](csg::Point2 const& r) {
      browser_->OnScreenResize(r.x, r.y);
   });

   lua_State* L = scriptHost_->GetInterpreter();
   renderer.SetScriptHost(GetScriptHost());
   om::RegisterLuaTypes(L);
   csg::RegisterLuaTypes(L);
   store_.SetInterpreter(L); // xxx move to dm open or something
   authoringStore_.SetInterpreter(L); // xxx move to dm open or something
   lua::om::register_json_to_lua_objects(L, store_);
   lua::om::register_json_to_lua_objects(L, authoringStore_);
   lua::client::open(L);
   lua::res::open(L);
   lua::voxel::open(L);
   lua::rpc::open(L, core_reactor_);


   //luabind::globals(L)["_client"] = luabind::object(L, this);

   // this locks down the environment!  all types must be registered by now!!
   scriptHost_->Require("radiant.client");

   _commands[GLFW_KEY_F10] = [&renderer, this]() {
      perf_hud_shown_ = !perf_hud_shown_;
      renderer.ShowPerfHud(perf_hud_shown_);
   };
   _commands[GLFW_KEY_F9] = [=]() { core_reactor_->Call(rpc::Function("radiant:toggle_debug_nodes")); };
   _commands[GLFW_KEY_F3] = [=]() { core_reactor_->Call(rpc::Function("radiant:toggle_step_paths")); };
   _commands[GLFW_KEY_F4] = [=]() { core_reactor_->Call(rpc::Function("radiant:step_paths")); };
   _commands[GLFW_KEY_ESCAPE] = [=]() {
      currentCursor_ = NULL;
   };

   // _commands[VK_NUMPAD0] = std::shared_ptr<command>(new command_build_blueprint(*_proxy_manager, *_renderer, 500));

   setup_connections();
   InitializeModules();

   _client_interval_start = platform::get_current_time_in_ms();
   _server_last_update_time = 0;

   while (renderer.IsRunning()) {
      static int last_stat_dump = 0;
      mainloop();
   }
}

void Client::InitializeModules()
{
   auto& rm = res::ResourceManager2::GetInstance();
   for (std::string const& modname : rm.GetModuleNames()) {
      try {
         json::Node manifest = rm.LookupManifest(modname);
         json::Node const& block = manifest.getn("client");
         if (!block.empty()) {
            LOG(WARNING) << "loading init script for " << modname << "...";
            LoadModuleInitScript(block);
         }
      } catch (std::exception const& e) {
         LOG(WARNING) << "load failed: " << e.what();
      }
   }
}

void Client::LoadModuleInitScript(json::Node const& block)
{
   std::string filename = block.get<std::string>("init_script", "");
   if (!filename.empty()) {
      scriptHost_->RequireScript(filename);
   }
}

static std::string SanatizePath(std::string const& path)
{
   return std::string("/") + strutil::join(strutil::split(path, "/"), "/");
}

void Client::handle_connect(const boost::system::error_code& error)
{
   if (error) {
      LOG(WARNING) << "connection to server failed (" << error << ").  retrying...";
      setup_connections();
   } else {
      recv_queue_->Read();
   }
}

void Client::setup_connections()
{
   recv_queue_ = std::make_shared<protocol::RecvQueue>(_tcp_socket);
   send_queue_ = protocol::SendQueue::Create(_tcp_socket);

   boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 8888);
   _tcp_socket.async_connect(endpoint, std::bind(&Client::handle_connect, this, std::placeholders::_1));
}

void Client::mainloop()
{
   perfmon::FrameGuard frame_guard;

   process_messages();
   ProcessBrowserJobQueue();   

   int currentTime = platform::get_current_time_in_ms();
   float alpha = (currentTime - _client_interval_start) / (float)_server_interval_duration;

   alpha = std::min(1.0f, std::max(alpha, 0.0f));
   now_ = (int)(_server_last_update_time + (_server_interval_duration * alpha));

   http_reactor_->FlushEvents();
   if (browser_) {
      perfmon::SwitchToCounter("browser poll");
      browser_->Work();
      auto cb = [](const csg::Region2 &rgn, const char* buffer) {
         Renderer::GetInstance().UpdateUITexture(rgn, buffer);
      };
      perfmon::SwitchToCounter("browser poll");
      browser_->UpdateDisplay(cb);
   }

   InstallCurrentCursor();
   HilightMouseover();

   // Fire the authoring traces *after* pumping the chrome message loop, since
   // we may create or modify authoring objects as a result of input events
   // or calls from the browser.
   perfmon::SwitchToCounter("fire traces");
   authoringStore_.FireTraces();
   authoringStore_.FireFinishedTraces();

   perfmon::SwitchToCounter("render");
   Renderer::GetInstance().RenderOneFrame(now_, alpha);

   if (send_queue_) {
      perfmon::SwitchToCounter("send msgs");
      protocol::SendQueue::Flush(send_queue_);
   }

   // xxx: GC while waiting for vsync, or GC in another thread while rendering (ooh!)
   perfmon::SwitchToCounter("lua gc");
   platform::timer t(10);
   scriptHost_->GC(t);
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

   _server_last_update_time = 0;
   _server_interval_duration = 1;
   _client_interval_start = 0;

   rootObject_ = NULL;
   selectedObject_ = NULL;
   objects_.clear();
}

bool Client::ProcessMessage(const proto::Update& msg)
{
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
      DISPATCH_MSG(PostCommandReply);
   default:
      ASSERT(false);
   }

   return true;
}

void Client::PostCommandReply(const proto::PostCommandReply& msg)
{
   protobuf_router_->OnPostCommandReply(msg);
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
   if (!rootObject_) {
      auto rootEntity = GetStore().FetchObject<om::Entity>(1);
      if (rootEntity) {
         rootObject_ = rootEntity;
         Renderer::GetInstance().Initialize(rootObject_);
         octtree_->SetRootEntity(rootEntity);
      }
   }

   // Only fire the remote store traces at sequence boundaries.  The
   // data isn't guaranteed to be in a consistent state between
   // boundaries.
   store_.FireTraces();
   store_.FireFinishedTraces();
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
      ASSERT(!store_.FetchStaticObject(id));

      objects_[id] = store_.AllocSlaveObject(entry.object_type(), id);
   }
}

void Client::UpdateObject(const proto::UpdateObject& update)
{
   const auto& msg = update.object();
   dm::ObjectId id = msg.object_id();

   dm::Object* obj = store_.FetchStaticObject(id);
   ASSERT(obj);
   obj->LoadObject(msg);
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
         ASSERT(i->second.use_count() <= 2);
      }

      if (i != objects_.end()) {
         objects_.erase(i);
      }

   }
}

void Client::UpdateDebugShapes(const proto::UpdateDebugShapes& msg)
{
   Renderer::GetInstance().DecodeDebugShapes(msg.shapelist());
}

void Client::process_messages()
{
   perfmon::TimelineCounterGuard tcg("process msgs") ;

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
   if (_server_last_update_time) {
      _server_interval_duration = time - _server_last_update_time;
   }
   _server_last_update_time = time;

   // We want to interpolate from the current point to the new destiation over
   // the interval returned by the server.
   _client_interval_start = platform::get_current_time_in_ms();
}

void Client::OnInput(Input const& input) {
   try {
      if (input.type == Input::MOUSE) {
         OnMouseInput(input);
      } else if (input.type == Input::KEYBOARD) {
         OnKeyboardInput(input);
      } else if (input.type == Input::RAW_INPUT) {
         OnRawInput(input);
      }
   } catch (std::exception &e) {
      LOG(WARNING) << "error dispatching input: " << e.what();
   }
}

void Client::OnMouseInput(Input const& input)
{
   bool handled = false, uninstall = false;

   mouse_x_ = input.mouse.x;
   mouse_y_ = input.mouse.y;

   browser_->OnInput(input);

   if (browser_->HasMouseFocus(input.mouse.x, input.mouse.y)) {
      return;
   }

   if (!CallInputHandlers(input)) {
      if (rootObject_ && input.mouse.up[0]) {
         LOG(WARNING) << "updating selection...";
         UpdateSelection(input.mouse);
      }
   }
}

void Client::OnKeyboardInput(Input const& input)
{
   if (input.keyboard.down) {
      auto i = _commands.find(input.keyboard.key);
      if (i != _commands.end()) {
         i->second();
         return;
      }
   }
   browser_->OnInput(input);
   CallInputHandlers(input);
}

void Client::OnRawInput(Input const& input)
{
   browser_->OnInput(input);
   CallInputHandlers(input);
}


Client::InputHandlerId Client::AddInputHandler(InputHandlerCb const& cb)
{
   InputHandlerId id = ReserveInputHandler();
   SetInputHandler(id, cb);
   return id;
}

void Client::SetInputHandler(InputHandlerId id, InputHandlerCb const& cb)
{
   input_handlers_.emplace_back(std::make_pair(id, cb));
}

Client::InputHandlerId Client::ReserveInputHandler() {
   return next_input_id_++;
}

void Client::RemoveInputHandler(InputHandlerId id)
{
   auto i = input_handlers_.begin();
   while (i != input_handlers_.end()) {
      if (i->first == id) {
         input_handlers_.erase(i);
         break;
      } else {
         i++;
      }
   }
};

bool Client::CallInputHandlers(Input const& input)
{
   auto handlers = input_handlers_;
   for (int i = (int)handlers.size() - 1; i >= 0; i--) {
      const auto& cb = handlers[i].second;
      if (cb && cb(input)) {
         return true;
      }
   }
   return false;
}

void Client::UpdateSelection(const MouseInput &mouse)
{
   om::Selection s;
   Renderer::GetInstance().QuerySceneRay(mouse.x, mouse.y, s);

   csg::Ray3 r;
   Renderer::GetInstance().GetCameraToViewportRay(mouse.x, mouse.y, &r);
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


void Client::SelectEntity(dm::ObjectId id)
{
   auto i = objects_.find(id);
   if (i != objects_.end() && i->second->GetObjectType() == om::EntityObjectType) {      
      SelectEntity(std::static_pointer_cast<om::Entity>(i->second));
   }
}

void Client::SelectEntity(om::EntityPtr obj)
{
   if (selectedObject_ != obj) {
      if (obj && obj->GetStore().GetStoreId() != GetStore().GetStoreId()) {
         LOG(WARNING) << "ignoring selected object with non-client store id.";
         return;
      }

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
         std::string uri = om::ObjectFormatter().GetPathToObject(selectedObject_);
         data.push_back(JSONNode("selected_entity", uri));
      }
      http_reactor_->QueueEvent("selection_changed.radiant", data);
   }
}

om::EntityPtr Client::GetEntity(dm::ObjectId id)
{
   auto obj = objects_[id];
   return (obj && obj->GetObjectType() == om::Entity::DmType) ? std::static_pointer_cast<om::Entity>(obj) : nullptr;
}

void Client::InstallCurrentCursor()
{
   if (browser_) {
      HCURSOR cursor;
      if (browser_->HasMouseFocus(mouse_x_, mouse_y_)) {
         cursor = uiCursor_;
      } else {
         if (!cursor_stack_.empty()) {
            cursor = cursor_stack_.back().second.get();
         } else if (!hilightedObjects_.empty()) {
            cursor = hover_cursor_.get();
         } else {
            cursor = default_cursor_.get();
         }
      }
      if (cursor != currentCursor_) {
         Renderer& renderer = Renderer::GetInstance();
         HWND hwnd = renderer.GetWindowHandle();

         SetClassLong(hwnd, GCL_HCURSOR, static_cast<LONG>(reinterpret_cast<LONG_PTR>(cursor)));
         ::SetCursor(cursor);
         currentCursor_ = cursor;
      }
   }
}

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

Client::Cursor Client::LoadCursor(std::string const& path)
{
   Cursor cursor;

   std::string filename = res::ResourceManager2::GetInstance().GetResourceFileName(path, ".cur");
   auto i = cursors_.find(filename);
   if (i != cursors_.end()) {
      cursor = i->second;
   } else {
      HCURSOR hcursor = (HCURSOR)LoadImageA(GetModuleHandle(NULL), filename.c_str(), IMAGE_CURSOR, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
      if (hcursor) {
         cursors_[filename] = cursor = Cursor(hcursor);
      }
   }
   return cursor;
}

Client::CursorStackId Client::InstallCursor(std::string name)
{
   CursorStackId id = next_cursor_stack_id_++;
   cursor_stack_.emplace_back(std::make_pair(id, LoadCursor(name)));
   return id;
}


void Client::RemoveCursor(CursorStackId id)
{
   for (auto i = cursor_stack_.begin(); i != cursor_stack_.end(); i++) {
      if (i->first == id) {
         cursor_stack_.erase(i);
         return;
      }
   }
}

rpc::ReactorDeferredPtr Client::GetModules(rpc::Function const& fn)
{
   rpc::ReactorDeferredPtr d = std::make_shared<rpc::ReactorDeferred>(fn.route);
   JSONNode result;
   auto& rm = res::ResourceManager2::GetInstance();
   for (std::string const& modname : rm.GetModuleNames()) {
      JSONNode manifest;
      try {
         manifest = rm.LookupManifest(modname).GetNode();
      } catch (std::exception const&) {
         // Just use an empty manifest...f
      }
      manifest.set_name(modname);
      result.push_back(manifest);
   }
   d->Resolve(result);
   return d;
}

// This function is called on the chrome thread!
void Client::BrowserRequestHandler(std::string const& path, json::Node const& query, std::string const& postdata, rpc::HttpDeferredPtr response)
{
   try {
      // file requests can be dispatched immediately.
      if (!boost::starts_with(path, "/r/")) {
         int code;
         std::string content, mimetype;
         if (http_reactor_->HttpGetFile(path, code, content, mimetype)) {
            response->Resolve(rpc::HttpResponse(code, content, mimetype));
         } else {
            response->Reject(rpc::HttpError(code, content));
         }
      } else {
         std::lock_guard<std::mutex> guard(browserJobQueueLock_);
         browserJobQueue_.emplace_back([=]() {
            CallHttpReactor(path, query, postdata, response);
         });
      }
   } catch (std::exception &e) {
      JSONNode fail;
      fail.push_back(JSONNode("error", e.what()));
      response->Reject(rpc::HttpError(500, fail.write()));
   }
}

void Client::CallHttpReactor(std::string path, json::Node query, std::string postdata, rpc::HttpDeferredPtr response)
{
   JSONNode node;
   int status = 404;
   rpc::ReactorDeferredPtr d;

   std::smatch match;
   if (std::regex_match(path, match, call_path_regex__)) {
      d = http_reactor_->Call(query, postdata);
   }
   if (!d) {
      response->Reject(rpc::HttpError(500, BUILD_STRING("failed to dispatch " << query)));
      return;
   }
   d->Progress([response](JSONNode n) {
      LOG(WARNING) << "error: got progress from deferred meant for rpc::HttpDeferredPtr! (logical error)";
      JSONNode result;
      n.set_name("data");
      result.push_back(JSONNode("error", "incomplete response"));
      result.push_back(n);
      response->Reject(rpc::HttpError(500, result.write()));
   });
   d->Done([response](JSONNode const& n) {
      response->Resolve(rpc::HttpResponse(200, n.write(), "application/json"));
   });
   d->Fail([response](JSONNode const& n) {
      response->Reject(rpc::HttpError(500, n.write()));
   });
   return;
}

om::EntityPtr Client::CreateEmptyAuthoringEntity()
{
   om::EntityPtr entity = authoringStore_.AllocObject<om::Entity>();   
   authoredEntities_[entity->GetObjectId()] = entity;
   return entity;
}

om::EntityPtr Client::CreateAuthoringEntity(std::string const& uri)
{
   om::EntityPtr entity = CreateEmptyAuthoringEntity();
   om::Stonehearth::InitEntity(entity, uri, nullptr);
   return entity;
}

void Client::DestroyAuthoringEntity(dm::ObjectId id)
{
   auto i = authoredEntities_.find(id);
   if (i != authoredEntities_.end()) {
      authoredEntities_.erase(i);
   }
}


void Client::ProcessBrowserJobQueue()
{
   perfmon::TimelineCounterGuard tcg("process job queue") ;

   std::lock_guard<std::mutex> guard(browserJobQueueLock_);
   for (const auto &fn : browserJobQueue_) {
      fn();
   }
   browserJobQueue_.clear();
}
