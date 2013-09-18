#include "pch.h"
#include <regex>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
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
#include "renderer/render_entity.h"
#include "renderer/lua_render_entity.h"
#include "renderer/lua_renderer.h"
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
#include "lib/rpc/trace_object_router.h"
#include "lib/rpc/http_deferred.h" // xxx: does not belong in rpc!
#include "lua/client/open.h"
#include "lua/rpc/open.h"
#include "lua/om/open.h"

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
   _server_last_update_time(0),
   _server_interval_duration(1),
   selectedObject_(NULL),
   last_sequence_number_(-1),
   rootObject_(NULL),
   store_(2, "game"),
   authoringStore_(3, "tmp"),
   nextTraceId_(1),
   uiCursor_(NULL),
   luaCursor_(NULL),
   currentCursor_(NULL),
   last_server_request_id_(0)
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
   rpc::TraceObjectRouterPtr trace_router = std::make_shared<rpc::TraceObjectRouter>(GetStore());
   core_reactor_->AddRouter(trace_router);

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
   core_reactor_->AddRoute("radiant.get_modules", [this](rpc::Function const& f) {
      return GetModules(f);
   });
   core_reactor_->AddRoute("radiant.install_trace", [this](rpc::Function const& f) {
      std::string uri = f.args.get<std::string>(0);
      return core_reactor_->InstallTrace(rpc::Trace(f.caller, f.call_id, uri));
   });
   core_reactor_->AddRoute("radiant.remove_trace", [this](rpc::Function const& f) {
      return core_reactor_->RemoveTrace(rpc::UnTrace(f.caller, f.call_id));
   });
}

Client::~Client()
{
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
   //renderer.SetCurrentPipeline("pipelines/deferred_pipeline_static.xml");
   //renderer.SetCurrentPipeline("pipelines/forward.pipeline.xml");

   Horde3D::Modules::log().SetNotifyErrorCb([=](om::ErrorBrowser::Record const& r) {
      error_browser_->AddRecord(r);
   });

   HWND hwnd = renderer.GetWindowHandle();
   //defaultCursor_ = (HCURSOR)GetClassLong(hwnd_, GCL_HCURSOR);

   renderer.SetMouseInputCallback(std::bind(&Client::OnMouseInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
   renderer.SetKeyboardInputCallback(std::bind(&Client::OnKeyboardInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

   int ui_width = renderer.GetUIWidth();
   int ui_height = renderer.GetUIHeight();
   int debug_port = 1338;

   namespace po = boost::program_options;
   extern po::variables_map configvm;
   std::string loader = configvm["game.loader"].as<std::string>().c_str();
   json::ConstJsonObject manifest(res::ResourceManager2::GetInstance().LookupManifest(loader));
   std::string docroot = "http://radiant/" + manifest.getn("loader").getn("ui").get<std::string>("homepage");

   if (configvm["game.script"].as<std::string>() != "stonehearth/new_world.lua") {
      docroot += "?skip_title=true";
   }

   browser_.reset(chromium::CreateBrowser(hwnd, docroot, ui_width, ui_height, debug_port));
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

   auto requestHandler = [=](std::string const& uri, JSONNode const& query, std::string const& postdata, rpc::HttpDeferredPtr response) {
      BrowserRequestHandler(uri, query, postdata, response);
   };
   browser_->SetRequestHandler(requestHandler);

   auto onRawInput = [=](RawInputEvent const & re, bool& handled, bool& uninstall) {
      browser_->OnRawInput(re, handled, uninstall);
   };

   auto onMouse = [=](MouseEvent const & windowMouse, MouseEvent const & browserMouse, bool& handled, bool& uninstall) {
      browser_->OnMouseInput(browserMouse, handled, uninstall);
   };
   renderer.SetRawInputCallback(onRawInput);
   renderer.SetMouseInputCallback(onMouse);


   lua_State* L = scriptHost_->GetInterpreter();
   renderer.SetScriptHost(GetScriptHost());
   lua::RegisterBasicTypes(L);
   om::RegisterLuaTypes(L);
   csg::RegisterLuaTypes(L);
   client::RegisterLuaTypes(L);
   store_.SetInterpreter(L); // xxx move to dm open or something
   authoringStore_.SetInterpreter(L); // xxx move to dm open or something
   lua::om::register_json_to_lua_objects(L, store_);
   lua::om::register_json_to_lua_objects(L, authoringStore_);

   luabind::globals(L)["_client"] = luabind::object(L, this);

   json::ConstJsonObject::RegisterLuaType(L);

   // this locks down the environment!  all types must be registered by now!!
   scriptHost_->Require("radiant.client");

#if 0
   // xxx: use Call on the reactor for this stuff...
   _commands[GLFW_KEY_F9] = std::bind(&Client::EvalCommand, this, "radiant.toggle_debug_nodes");
   _commands[GLFW_KEY_F3] = std::bind(&Client::EvalCommand, this, "radiant.toggle_step_paths");
   _commands[GLFW_KEY_F4] = std::bind(&Client::EvalCommand, this, "radiant.step_paths");
#endif
   _commands[GLFW_KEY_ESC] = [=]() {
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
      int now = platform::get_current_time_in_ms();
      if (now - last_stat_dump > 10000) {
         PROFILE_DUMP_STATS();
         // PROFILE_RESET();
         last_stat_dump = now;
      }
   }
}

void Client::InitializeModules()
{
   auto& rm = res::ResourceManager2::GetInstance();
   for (std::string const& modname : rm.GetModuleNames()) {
      try {
         json::ConstJsonObject manifest = rm.LookupManifest(modname);
         json::ConstJsonObject const& block = manifest.getn("client");
         if (!block.empty()) {
            LOG(WARNING) << "loading init script for " << modname << "...";
            LoadModuleInitScript(block);
         }
      } catch (std::exception const& e) {
         LOG(WARNING) << "load failed: " << e.what();
      }
   }
}

void Client::LoadModuleInitScript(json::ConstJsonObject const& block)
{
   std::string filename = block.get<std::string>("init_script");
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
   PROFILE_BLOCK();
   
   process_messages();
   ProcessBrowserJobQueue();

   int currentTime = platform::get_current_time_in_ms();
   float alpha = (currentTime - _client_interval_start) / (float)_server_interval_duration;

   alpha = std::min(1.0f, std::max(alpha, 0.0f));
   now_ = (int)(_server_last_update_time + (_server_interval_duration * alpha));

   http_reactor_->FlushEvents();
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
   // xxx: GC while waiting for vsync, or GC in another thread while rendering (ooh!)
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

   //LOG(WARNING) << "Client updating object " << id << ".";

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

void Client::OnMouseInput(const MouseEvent &windowMouse, const MouseEvent &browserMouse, bool &handled, bool &uninstall)
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
         promise->OnMouseEvent(windowMouse, handled);
         i++;
      } else {
         i = mouseEventPromises_.erase(i);
      }
   }

   if (!handled) {
      if (rootObject_ && windowMouse.up[0]) {
         LOG(WARNING) << "updating selection...";
         UpdateSelection(windowMouse);
      } else if (windowMouse.up[2]) {
         CenterMap(windowMouse);
      }
   }
}

void Client::CenterMap(const MouseEvent &mouse)
{
   om::Selection s;

   Renderer::GetInstance().QuerySceneRay(mouse.x, mouse.y, s);
   if (s.HasBlock()) {
      Renderer::GetInstance().PlaceCamera(csg::ToFloat(s.GetBlock()));
   }
}

void Client::UpdateSelection(const MouseEvent &mouse)
{
   om::Selection s;
   Renderer::GetInstance().QuerySceneRay(mouse.x, mouse.y, s);

   csg::Ray3 r = Renderer::GetInstance().GetCameraToViewportRay(mouse.x, mouse.y);
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

void Client::InstallCursor()
{
   if (browser_) {
      HCURSOR cursor;
      if (browser_->HasMouseFocus()) {
         cursor = uiCursor_;
      } else {
         cursor = NULL;
         /* which cursor do we want? to be written */
         if (!cursor && luaCursor_) {
            cursor = luaCursor_;
         }
         if (!cursor && !hilightedObjects_.empty()) {
            cursor = cursors_["hover"].get();
         }
         if (!cursor) {
            cursor = cursors_["default"].get();
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

void Client::SetCursor(std::string name)
{
   auto i = cursors_.find(name);
   if (i != cursors_.end()) {
      currentCursor_ = i->second.get();
   } else {
      currentCursor_ = NULL;
   }
}

static void HandleFileRequest(std::string const& path, rpc::HttpDeferredPtr response)
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
   auto const& rm = res::ResourceManager2::GetInstance();

   std::string data;
   try {
      if (boost::ends_with(path, ".json")) {
         JSONNode const& node = rm.LookupJson(path);
         data = node.write();
      } else {
         LOG(WARNING) << "reading file " << path;
         rm.OpenResource(path, infile);
         data = io::read_contents(infile);
      }
   } catch (res::Exception const& e) {
      LOG(WARNING) << "error code 404: " << e.what();
      response->Reject(rpc::HttpError(404, e.what()));
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
   response->Resolve(rpc::HttpResponse(200, data, mimeType));
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
void Client::BrowserRequestHandler(std::string const& path, json::ConstJsonObject const& query, std::string const& postdata, rpc::HttpDeferredPtr response)
{
   try {
      // file requests can be dispatched immediately.
      if (!boost::starts_with(path, "/r/")) {
         HandleFileRequest(path, response);
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

void Client::CallHttpReactor(std::string path, json::ConstJsonObject query, std::string postdata, rpc::HttpDeferredPtr response)
{
   JSONNode node;
   int status = 404;
   rpc::ReactorDeferredPtr d;

   static std::regex call_path("/r/call/?");
   std::smatch match;
   if (std::regex_match(path, match, call_path)) {
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


om::EntityPtr Client::CreateAuthoringEntity(std::string const& mod_name, std::string const& entity_name)
{
   om::EntityPtr entity = CreateEmptyAuthoringEntity();
   om::Stonehearth::InitEntity(entity, mod_name, entity_name, nullptr);
   return entity;
}

om::EntityPtr Client::CreateAuthoringEntityByRef(std::string const& ref)
{
   om::EntityPtr entity = CreateEmptyAuthoringEntity();
   om::Stonehearth::InitEntityByRef(entity, ref, nullptr);
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

void Client::ProcessBrowserJobQueue()
{
   std::lock_guard<std::mutex> guard(browserJobQueueLock_);
   for (const auto &fn : browserJobQueue_) {
      fn();
   }
   browserJobQueue_.clear();
}


void client::RegisterLuaTypes(lua_State* L)
{
   LuaRenderer::RegisterType(L);
   LuaRenderEntity::RegisterType(L);
   Client::RegisterLuaTypes(L);
   lua::client::open(L);
   lua::rpc::open(L);
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

static luabind::object
Client_QueryScene(lua_State* L, Client const&, int x, int y)
{
   om::Selection s;
   Renderer::GetInstance().QuerySceneRay(x, y, s);

   using namespace luabind;
   object result = newtable(L);
   if (s.HasBlock()) {
      result["location"] = object(L, s.GetBlock());
      result["normal"]   = object(L, csg::ToInt(s.GetNormal()));
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
      dm::ObjectPtr obj =  om::ObjectFormatter().GetObject(store, path);
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

static bool MouseEvent_GetDown(MouseEvent const& me, int button)
{
   button--;
   if (button >= 0 && button < sizeof(me.down)) {
      return me.down[button];
   }
   return false;
}

#if 0
std::shared_ptr<LuaDeferred2<XZRegionSelector::Deferred>> Client_SelectXZRegion(lua_State* L, Client& c)
{
   auto s = std::make_shared<XZRegionSelector>(c.GetTerrain());
   auto deferred = s->Activate();
   auto luaDeferred = std::make_shared<LuaDeferred2<XZRegionSelector::Deferred>>(c.GetScriptHost(), deferred);
   return luaDeferred;
}
#endif

om::EntityRef Client_CreateEmptyAuthoringEntity(Client& client)
{
   return client.CreateEmptyAuthoringEntity();
}

om::EntityRef Client_CreateAuthoringEntity(Client& client, std::string const& mod_name, std::string const& entity_name)
{
   return client.CreateAuthoringEntity(mod_name, entity_name);
}

om::EntityRef Client_CreateAuthoringEntityByRef(Client& client, std::string const& ref)
{
   return client.CreateAuthoringEntityByRef(ref);
}

template <class T>
luabind::scope RegisterLuaDeferredType(lua_State* L)
{
   return
      lua::RegisterTypePtr<T>()
         .def("done",     &T::LuaDone)
         .def("progress", &T::LuaProgress)
         .def("fail",     &T::LuaFail)
         .def("always",   &T::LuaAlways);
}

IMPLEMENT_TRIVIAL_TOSTRING(Client)
IMPLEMENT_TRIVIAL_TOSTRING(MouseEventPromise)
IMPLEMENT_TRIVIAL_TOSTRING(MouseEvent)

void Client::RegisterLuaTypes(lua_State* L)
{
   using namespace luabind;
   module(L) [
      lua::RegisterType<Client>()
         .def("create_empty_authoring_entity",  &Client_CreateEmptyAuthoringEntity)
         .def("create_authoring_entity",        &Client_CreateAuthoringEntity)
         .def("create_authoring_entity_by_ref", &Client_CreateAuthoringEntityByRef)
         .def("destroy_authoring_entity", &Client::DestroyAuthoringEntity)
         .def("create_render_entity",     &Client_CreateRenderEntity)
         .def("trace_mouse",              &Client::TraceMouseEvents)
         .def("query_scene",              &Client_QueryScene)
#if 0
         .def("select_xz_region",         &Client_SelectXZRegion)
         .def("call",                     &Client_Call)
#endif
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
         .def("down",             &MouseEvent_GetDown)
   ];
}
