#include "pch.h"
#include <regex>
#include "build_number.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include "core/config.h"
#include "radiant_file.h"
#include "radiant_exceptions.h"
#include "xz_region_selector.h"
#include "om/entity.h"
#include "om/components/terrain.ridl.h"
#include "om/error_browser/error_browser.h"
#include "om/selection.h"
#include "om/om_alloc.h"
#include "csg/lua/lua_csg.h"
#include "om/lua/lua_om.h"
#include "om/components/data_store.ridl.h"
#include "platform/utils.h"
#include "resources/manifest.h"
#include "resources/res_manager.h"
#include "om/stonehearth.h"
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
#include "lib/lua/register.h"
#include "lib/lua/script_host.h"
#include "lib/lua/client/open.h"
#include "lib/lua/res/open.h"
#include "lib/lua/rpc/open.h"
#include "lib/lua/om/open.h"
#include "lib/lua/dm/open.h"
#include "lib/lua/voxel/open.h"
#include "lib/lua/analytics/open.h"
#include "lib/lua/audio/open.h"
#include "lib/analytics/design_event.h"
#include "lib/analytics/post_data.h"
#include "lib/audio/input_stream.h"
#include "lib/audio/audio_manager.h"
#include "lib/audio/audio.h"
#include "client/renderer/render_entity.h"
#include "lib/perfmon/perfmon.h"
#include "platform/sysinfo.h"
#include "glfw3.h"
#include "dm/receiver.h"
#include "dm/tracer_sync.h"
#include "dm/tracer_buffered.h"

//  #include "GFx/AS3/AS3_Global.h"
#include "client.h"
#include "clock.h"
#include "renderer/renderer.h"
#include "libjson.h"

using namespace ::radiant;
using namespace ::radiant::client;
namespace fs = boost::filesystem;
namespace po = boost::program_options;
namespace proto = ::radiant::tesseract::protocol;

static const std::regex call_path_regex__("/r/call/?");

DEFINE_SINGLETON(Client);

template<typename T> 
json::Node makeRendererConfigNode(RendererConfigEntry<T>& e) {
   json::Node n;

   n.set("value", e.value);
   n.set("allowed", e.allowed);

   return n;
}

Client::Client() :
   _tcp_socket(_io_service),
   store_(nullptr),
   authoringStore_(nullptr),
   uiCursor_(NULL),
   currentCursor_(NULL),
   next_input_id_(1),
   mouse_x_(0),
   mouse_y_(0),
   perf_hud_shown_(false),
   connected_(false),
   game_clock_(nullptr),
   enable_debug_cursor_(false)
{
}

Client::~Client()
{
   Shutdown();
}

void Client::InitializeUI()
{
   core::Config const& config = core::Config::GetInstance();
   std::string main_mod = config.Get<std::string>("game.main_mod", "stonehearth");
   
   std::string docroot;
   res::ResourceManager2& resource_manager = res::ResourceManager2::GetInstance();
   resource_manager.LookupManifest(main_mod, [&](const res::Manifest& manifest) {
      docroot = manifest.get<std::string>("ui.homepage", "about:");
   });
   browser_->Navigate(docroot);
}

void Client::OneTimeIninitializtion()
{
   core::Config const& config = core::Config::GetInstance();

   // connect to the server...
   setup_connections();

   // renderer...
   Renderer& renderer = Renderer::GetInstance();
   HWND hwnd = renderer.GetWindowHandle();

   renderer.SetInputHandler([=](Input const& input) {
      OnInput(input);
   });

   // browser...
   int screen_width = renderer.GetWidth();
   int screen_height = renderer.GetHeight();

   browser_.reset(chromium::CreateBrowser(hwnd, "", screen_width, screen_height, 1338));
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
   browserResizeGuard_ = renderer.OnScreenResize([this](csg::Point2 const& r) {
      browser_->OnScreenResize(r.x, r.y);
   });

   if (config.Get("enable_debug_keys", false)) {
      _commands[GLFW_KEY_F1] = [this]() {
         enable_debug_cursor_ = !enable_debug_cursor_;
         CLIENT_LOG(0) << "debug cursor " << (enable_debug_cursor_ ? "ON" : "OFF");
         if (!enable_debug_cursor_) {
            json::Node args;
            args.set("enabled", false);
            core_reactor_->Call(rpc::Function("radiant:debug_navgrid", args));
         }
      };
      _commands[GLFW_KEY_F3] = [=]() { core_reactor_->Call(rpc::Function("radiant:toggle_step_paths")); };
      _commands[GLFW_KEY_F4] = [=]() { core_reactor_->Call(rpc::Function("radiant:step_paths")); };
      _commands[GLFW_KEY_F5] = [=]() { RequestReload(); };
      _commands[GLFW_KEY_F6] = [=]() { core_reactor_->Call(rpc::Function("radiant:server:save")); };
      _commands[GLFW_KEY_F7] = [=]() { core_reactor_->Call(rpc::Function("radiant:server:load")); };
      _commands[GLFW_KEY_F9] = [=]() { core_reactor_->Call(rpc::Function("radiant:toggle_debug_nodes")); };
      _commands[GLFW_KEY_F10] = [&renderer, this]() {
         perf_hud_shown_ = !perf_hud_shown_;
         renderer.ShowPerfHud(perf_hud_shown_);
      };
      _commands[GLFW_KEY_F12] = [&renderer]() {
         // Toggling this causes large memory leak in malloc (30 MB per toggle in a 25 tile world)
         renderer.SetShowDebugShapes(!renderer.ShowDebugShapes());
      };
      _commands[GLFW_KEY_PAUSE] = []() {
         // throw an exception that is not caught by Client::OnInput
         throw std::string("User hit crash key");
      };
      _commands[GLFW_KEY_NUM_LOCK] = [=]() { core_reactor_->Call(rpc::Function("radiant:profile_next_lua_upate")); };
      _commands[GLFW_KEY_KP_7] = [=]() { core_reactor_->Call(rpc::Function("radiant:start_lua_memory_profile")); };
      _commands[GLFW_KEY_KP_1] = [=]() { core_reactor_->Call(rpc::Function("radiant:stop_lua_memory_profile")); };
      _commands[GLFW_KEY_KP_DECIMAL] = [=]() { core_reactor_->Call(rpc::Function("radiant:clear_lua_memory_profile")); };
      _commands[GLFW_KEY_KP_ENTER] = [=]() { core_reactor_->Call(rpc::Function("radiant:write_lua_memory_profile")); };
      // _commands[VK_NUMPAD0] = std::shared_ptr<command>(new command_build_blueprint(*_proxy_manager, *_renderer, 500));
   }

   // Reactors...
   core_reactor_ = std::make_shared<rpc::CoreReactor>();
   http_reactor_ = std::make_shared<rpc::HttpReactor>(*core_reactor_);

   // Routers... 
   protobuf_router_ = std::make_shared<rpc::ProtobufRouter>([this](proto::PostCommandRequest const& request) {
      proto::Request msg;
      msg.set_type(proto::Request::PostCommandRequest);   
      proto::PostCommandRequest* r = msg.MutableExtension(proto::PostCommandRequest::extension);
      *r = request;

      send_queue_->Push(msg);
   });
   core_reactor_->SetRemoteRouter(protobuf_router_);

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
   core_reactor_->AddRoute("radiant:send_design_event", [this](rpc::Function const& f) {
      rpc::ReactorDeferredPtr result = std::make_shared<rpc::ReactorDeferred>("radiant:design_event");
      try {
         json::Node node(f.args);
         std::string event_name = node.get<std::string>(0);
         analytics::DesignEvent design_event(event_name);

         if (node.size() > 1) {
            json::Node params = node.get_node(1);
            if (params.has("value")) {
               design_event.SetValue(params.get<float>("value"));
            }
            if (params.has("position")) {
               //csg::Point3f pos = params.get<csg::Point3f>("position");
               design_event.SetPosition(params.get<csg::Point3f>("position"));
            }
            if (params.has("area")) {
               design_event.SetArea(params.get<std::string>("area"));
            }
         }
         design_event.SendEvent();
         result->ResolveWithMsg("success");
      } catch (std::exception const& e) {
         result->RejectWithMsg(BUILD_STRING("exception: " << e.what()));
      }
      //return GetModules(f);
      return result;
   });

  //Send information about this build of the client
   core_reactor_->AddRoute("radiant:client_about_info", [this](rpc::Function const& f) {
      rpc::ReactorDeferredPtr result = std::make_shared<rpc::ReactorDeferred>("radiant:client_about_info");
      try {
         json::Node node;
         core::Config& config = core::Config::GetInstance();
         node.set("product_name", PRODUCT_NAME);
         node.set("product_major_version", PRODUCT_MAJOR_VERSION);
         node.set("product_minor_version", PRODUCT_MINOR_VERSION);
         node.set("product_patch_version", PRODUCT_PATCH_VERSION);
         node.set("product_build_number", PRODUCT_BUILD_NUMBER);
         node.set("product_revision", PRODUCT_REVISION);
         node.set("product_branch", PRODUCT_BRANCH);
         node.set("product_version_string", PRODUCT_VERSION_STR);
         node.set("product_file_version_string", PRODUCT_FILE_VERSION_STR);
         result->Resolve(node);
      } catch (std::exception const& e) {
         result->RejectWithMsg(BUILD_STRING("exception: " << e.what()));
      }
      //return GetModules(f);
      return result;
   });

   //Allow user to opt in/out of analytics from JS
   core_reactor_->AddRoute("radiant:set_collection_status", [this](rpc::Function const& f) {
      rpc::ReactorDeferredPtr result = std::make_shared<rpc::ReactorDeferred>("radiant:design_event");
      try {
         json::Node node(f.args);
         bool collect_stats = node.get<bool>(0);
         analytics::SetCollectionStatus(collect_stats);
         result->ResolveWithMsg("success");
      } catch (std::exception const& e) {
         result->RejectWithMsg(BUILD_STRING("exception: " << e.what()));
      }
      //return GetModules(f);
      return result;
   });

   //Pass analytics status to JS
   //TODO: re-write as general settings getter/setters
   core_reactor_->AddRoute("radiant:get_collection_status", [this](rpc::Function const& f) {
      rpc::ReactorDeferredPtr result = std::make_shared<rpc::ReactorDeferred>("radiant:design_event");
      try {
         json::Node node;
         core::Config& config = core::Config::GetInstance();
         node.set("has_expressed_preference", analytics::IsCollectionStatusSet());
         node.set("collection_status", analytics::GetCollectionStatus());
         result->Resolve(node);
      } catch (std::exception const& e) {
         result->RejectWithMsg(BUILD_STRING("exception: " << e.what()));
      }
      return result;
   });

   //Pass framerate, cpu, card, memory info up
   core_reactor_->AddRoute("radiant:send_performance_stats", [this](rpc::Function const& f) {
      rpc::ReactorDeferredPtr result = std::make_shared<rpc::ReactorDeferred>("radiant:send_performance_stats");
      try {
         json::Node node;
         SystemStats stats = Renderer::GetInstance().GetStats();
         node.set("UserId", core::Config::GetInstance().GetUserID());
         node.set("FrameRate", stats.frame_rate);
         node.set("GpuVendor", stats.gpu_vendor);
         node.set("GpuRenderer", stats.gpu_renderer);
         node.set("GlVersion", stats.gl_version);
         node.set("CpuInfo", stats.cpu_info);
         node.set("MemoryGb", stats.memory_gb);
         node.set("OsName", platform::SysInfo::GetOSName());
         node.set("OsVersion", platform::SysInfo::GetOSVersion());

         // xxx, parse GAME_DEMOGRAPHICS_URL into domain and path, in postdata
         analytics::PostData post_data(node, REPORT_SYSINFO_URI,  "");
         post_data.Send();
         result->ResolveWithMsg("success");

      } catch (std::exception const& e) {
         result->RejectWithMsg(BUILD_STRING("exception: " << e.what()));
      }
      return result;
   });

   //TODO: take arguments to accomodate effects sounds
   core_reactor_->AddRoute("radiant:play_sound", [this](rpc::Function const& f) {
      rpc::ReactorDeferredPtr result = std::make_shared<rpc::ReactorDeferred>("radiant:play_sound");
      try {
         json::Node node(f.args);
         std::string sound_url = node.get_node(0).as<std::string>();
         audio::AudioManager &a = audio::AudioManager::GetInstance();
         a.PlaySound(sound_url);
         result->ResolveWithMsg("success");
      } catch (std::exception const& e) {
         result->RejectWithMsg(BUILD_STRING("exception: " << e.what()));
      }
      return result;
   });

   core_reactor_->AddRoute("radiant:play_music", [this](rpc::Function const& f) {
      rpc::ReactorDeferredPtr result = std::make_shared<rpc::ReactorDeferred>("radiant:play_bgm");
      try {         
         json::Node node(f.args);
         json::Node params = node.get_node(0);

         audio::AudioManager &a = audio::AudioManager::GetInstance();
         
         //Get track, channel, and other optional data out of the node
         //TODO: get the defaults from audio.cpp instead of 
         std::string uri = params.get<std::string>("track");
         std::string channel = params.get<std::string>("channel");

         bool loop = params.get<bool>("loop", audio::DEF_MUSIC_LOOP);
         a.SetNextMusicLoop(loop, channel);

         int fade = params.get<int>("fade", audio::DEF_MUSIC_FADE);
         a.SetNextMusicFade(fade, channel);

         int vol = params.get<int>("volume", audio::DEF_MUSIC_VOL);
         a.SetNextMusicVolume(vol, channel);

         bool crossfade = params.get<bool>("crossfade", audio::DEF_MUSIC_CROSSFADE);
         a.SetNextMusicCrossfade(crossfade, channel);
         
         a.PlayMusic(uri, channel);

         result->ResolveWithMsg("success");
      } catch (std::exception const& e) {
         result->RejectWithMsg(BUILD_STRING("exception: " << e.what()));
      }
      return result;
   });

   core_reactor_->AddRoute("radiant:exit", [this](rpc::Function const& f) {
	  TerminateProcess(GetCurrentProcess(), 0);
      return nullptr;
   });

   core_reactor_->AddRoute("radiant:set_draw_world", [this](rpc::Function const& f) {
      rpc::ReactorDeferredPtr result = std::make_shared<rpc::ReactorDeferred>("radiant:set_draw_world");

      try {
         json::Node node(f.args);
         json::Node params = node.get_node(0);

         if (params.has("draw_world")) {
            Renderer::GetInstance().SetDrawWorld(params.get<bool>("draw_world", true));
         }

         result->ResolveWithMsg("success");
      } catch (std::exception const& e) {
         result->RejectWithMsg(BUILD_STRING("exception: " << e.what()));
      }
      return result;
   });


   core_reactor_->AddRoute("radiant:get_config_options", [this](rpc::Function const& f) {
      rpc::ReactorDeferredPtr result = std::make_shared<rpc::ReactorDeferred>("radiant:get_config_options");
      try {
         json::Node node;
         RendererConfig cfg = Renderer::GetInstance().GetRendererConfig();
         
         node.set("shadows", makeRendererConfigNode(cfg.use_shadows));
         node.set("msaa", makeRendererConfigNode(cfg.num_msaa_samples));
         node.set("vsync", makeRendererConfigNode(cfg.enable_vsync));
         node.set("xres", makeRendererConfigNode(cfg.screen_width));
         node.set("yres", makeRendererConfigNode(cfg.screen_height));
         node.set("fullscreen", makeRendererConfigNode(cfg.enable_fullscreen));
         node.set("shadow_res", makeRendererConfigNode(cfg.shadow_resolution));
         node.set("draw_distance", makeRendererConfigNode(cfg.draw_distance));
         node.set("gfx_card_renderer", Renderer::GetInstance().GetStats().gpu_renderer);
         node.set("gfx_card_driver", Renderer::GetInstance().GetStats().gl_version);
         node.set("use_fast_hilite", makeRendererConfigNode(cfg.use_fast_hilite));
         node.set("enable_ssao", makeRendererConfigNode(cfg.enable_ssao));

         result->Resolve(node);
      } catch (std::exception const& e) {
         result->RejectWithMsg(BUILD_STRING("exception: " << e.what()));
      }
      return result;
   });

   core_reactor_->AddRoute("radiant:set_config_options", [this](rpc::Function const& f) {
      rpc::ReactorDeferredPtr result = std::make_shared<rpc::ReactorDeferred>("radiant:set_config_options");
      try {
         json::Node params(json::Node(f.args).get_node(0));
         RendererConfig oldCfg = Renderer::GetInstance().GetRendererConfig();
         RendererConfig newCfg;
         memcpy(&newCfg, &oldCfg, sizeof(RendererConfig));

         bool persistConfig = params.get<bool>("persistConfig", false);
         newCfg.use_shadows.value = params.get<bool>("shadows", oldCfg.use_shadows.value);
         newCfg.num_msaa_samples.value = params.get<int>("msaa", oldCfg.num_msaa_samples.value);
         newCfg.shadow_resolution.value = params.get<int>("shadow_res", oldCfg.shadow_resolution.value);
         newCfg.enable_fullscreen.value = params.get<bool>("fullscreen", oldCfg.enable_fullscreen.value);
         newCfg.enable_vsync.value = params.get<bool>("vsync", oldCfg.enable_vsync.value);
         newCfg.draw_distance.value = params.get<float>("draw_distance", oldCfg.draw_distance.value);
         newCfg.use_fast_hilite.value = params.get<bool>("use_fast_hilite", oldCfg.use_fast_hilite.value);
         newCfg.enable_ssao.value = params.get<bool>("enable_ssao", oldCfg.enable_ssao.value);
         
         Renderer::GetInstance().ApplyConfig(newCfg, persistConfig);

         result->ResolveWithMsg("success");
      } catch (std::exception const& e) {
         result->RejectWithMsg(BUILD_STRING("exception: " << e.what()));
      }
      return result;
   });

   core_reactor_->AddRoute("radiant:client:get_error_browser", [this](rpc::Function const& f) {
      rpc::ReactorDeferredPtr d = std::make_shared<rpc::ReactorDeferred>("get error browser");
      json::Node obj;
      obj.set("error_browser", error_browser_->GetStoreAddress());
      d->Resolve(obj);
      return d;
   });

   core_reactor_->AddRouteV("radiant:client:deactivate_all_tools", [this](rpc::Function const& f) {
      DeactivateAllTools();
   });

}

void Client::Initialize()
{
   InitializeDataObjects();
   InitializeGameObjects();
   InitializeLuaObjects();
}

void Client::Shutdown()
{
   ShutdownLuaObjects();
   ShutdownGameObjects();
   ShutdownDataObjects();
}

void Client::InitializeDataObjects()
{
   store_.reset(new dm::Store(2, "game"));
   authoringStore_.reset(new dm::Store(3, "tmp"));

   om::RegisterObjectTypes(*store_);
   om::RegisterObjectTypes(*authoringStore_);

   receiver_ = std::make_shared<dm::Receiver>(*store_);

   game_render_tracer_ = std::make_shared<dm::TracerBuffered>("client render", *store_);
   authoring_render_tracer_ = std::make_shared<dm::TracerBuffered>("client tmp render", *authoringStore_);
   store_->AddTracer(game_render_tracer_, dm::RENDER_TRACES);
   store_->AddTracer(game_render_tracer_, dm::LUA_SYNC_TRACES);
   store_->AddTracer(game_render_tracer_, dm::LUA_ASYNC_TRACES);
   store_->AddTracer(game_render_tracer_, dm::RPC_TRACES);

   object_model_traces_ = std::make_shared<dm::TracerSync>("client om");
   authoringStore_->AddTracer(authoring_render_tracer_, dm::RENDER_TRACES);
   authoringStore_->AddTracer(authoring_render_tracer_, dm::LUA_SYNC_TRACES);
   authoringStore_->AddTracer(authoring_render_tracer_, dm::LUA_ASYNC_TRACES);
   authoringStore_->AddTracer(authoring_render_tracer_, dm::RPC_TRACES);
   authoringStore_->AddTracer(object_model_traces_, dm::OBJECT_MODEL_TRACES);
}

void Client::ShutdownDataObjects()
{
   game_render_tracer_.reset();
   authoring_render_tracer_.reset();
   object_model_traces_.reset();

   receiver_.reset();
   authoringStore_.reset();
   store_.reset();
}

void Client::InitializeGameObjects()
{
   Renderer& renderer = Renderer::GetInstance();

   renderer.Initialize();

   octtree_.reset(new phys::OctTree(dm::RENDER_TRACES));
   scriptHost_.reset(new lua::ScriptHost());

   error_browser_ = authoringStore_->AllocObject<om::ErrorBrowser>();
   scriptHost_.reset(new lua::ScriptHost());
   scriptHost_->SetNotifyErrorCb([=](om::ErrorBrowser::Record const& r) {
      error_browser_->AddRecord(r);
   });
   luaModuleRouter_ = std::make_shared<rpc::LuaModuleRouter>(scriptHost_.get(), "client");
   luaObjectRouter_ = std::make_shared<rpc::LuaObjectRouter>(scriptHost_.get(), GetAuthoringStore());
   traceObjectRouter_ = std::make_shared<rpc::TraceObjectRouter>(*store_);
   traceAuthoredObjectRouter_ = std::make_shared<rpc::TraceObjectRouter>(*authoringStore_);

   core_reactor_->AddRouter(traceObjectRouter_);
   core_reactor_->AddRouter(traceAuthoredObjectRouter_);
   core_reactor_->AddRouter(luaModuleRouter_);
   core_reactor_->AddRouter(luaObjectRouter_);

   Horde3D::Modules::log().SetNotifyErrorCb([=](om::ErrorBrowser::Record const& r) {
      error_browser_->AddRecord(r);
   });
   game_clock_.reset(new Clock());

   lua_State* L = scriptHost_->GetInterpreter();
   lua_State* callback_thread = scriptHost_->GetCallbackThread();
   renderer.SetScriptHost(GetScriptHost());
   csg::RegisterLuaTypes(L);
   store_->SetInterpreter(callback_thread); // xxx move to dm open or something
   authoringStore_->SetInterpreter(callback_thread); // xxx move to dm open or something
   lua::om::register_json_to_lua_objects(L, *store_);
   lua::om::register_json_to_lua_objects(L, *authoringStore_);
   lua::om::open(L);
   lua::dm::open(L);
   lua::client::open(L);
   lua::res::open(L);
   lua::voxel::open(L);
   lua::rpc::open(L, core_reactor_);
   lua::analytics::open(L);
   lua::audio::open(L);
}

void Client::ShutdownGameObjects()
{
   Renderer::GetInstance().Shutdown();
   rootObject_.reset();
   hilightedObjects_.clear();
   authoredEntities_.clear();
   
   game_clock_.reset();
   Horde3D::Modules::log().SetNotifyErrorCb(nullptr);
   scriptHost_->SetNotifyErrorCb(nullptr);

   core_reactor_->RemoveRouter(luaModuleRouter_);
   core_reactor_->RemoveRouter(luaObjectRouter_);
   core_reactor_->RemoveRouter(traceObjectRouter_);
   core_reactor_->RemoveRouter(traceAuthoredObjectRouter_);

   luaModuleRouter_ = nullptr;
   luaObjectRouter_ = nullptr;
   traceObjectRouter_ = nullptr;
   traceAuthoredObjectRouter_ = nullptr;

   error_browser_.reset();
   scriptHost_.reset();
   octtree_.reset();
}

void Client::InitializeLuaObjects()
{
   core::Config &config = core::Config::GetInstance();
   res::ResourceManager2& resource_manager = res::ResourceManager2::GetInstance();
   lua_State* L = scriptHost_->GetInterpreter();

   // xxx: the module init sequnce on the client and server share a ton of code.  refactor!!
   luabind::object main_mod_obj;
   std::string const main_mod_name = config.Get<std::string>("game.main_mod", "stonehearth");

   hover_cursor_ = LoadCursor("stonehearth:cursors:hover");
   default_cursor_ = LoadCursor("stonehearth:cursors:default");
   
   radiant_ = scriptHost_->Require("radiant.client");
   for (std::string const& mod_name : resource_manager.GetModuleNames()) {
      std::string script_name;
      resource_manager.LookupManifest(mod_name, [&](const res::Manifest& manifest) {
         script_name = manifest.get<std::string>("client_init_script", "");
      });
      if (!script_name.empty()) {
         try {
            luabind::object obj = scriptHost_->Require(script_name);
            if (obj) {
               luabind::globals(L)[mod_name] = obj;
               if (mod_name == main_mod_name) {
                  main_mod_obj = obj;
               }
            }
         } catch (std::exception const& e) {
            CLIENT_LOG(1) << "module " << mod_name << " failed to load " << script_name << ": " << e.what();
         }
      }
   }
   // this locks down the environment!  all types must be registered by now!!
   scriptHost_->Require("radiant.lualibs.strict");
   scriptHost_->Trigger("radiant:modules_loaded");

   if (main_mod_obj) {
      scriptHost_->TriggerOn(main_mod_obj, "radiant:new_game", luabind::newtable(L));
   }
}

void Client::ShutdownLuaObjects()
{
   // keep the cursors around between save/load...
}

void Client::run(int server_port)
{
   Renderer& renderer = Renderer::GetInstance();
   
   server_port_ = server_port;

   OneTimeIninitializtion();
   Initialize();
   InitializeUI();

   while (renderer.IsRunning()) {
      perfmon::BeginFrame(perf_hud_shown_);
      perfmon::TimelineCounterGuard tcg("client run loop") ;
      mainloop();
   }
}

static std::string SanatizePath(std::string const& path)
{
   return std::string("/") + strutil::join(strutil::split(path, "/"), "/");
}

void Client::handle_connect(const boost::system::error_code& error)
{
   if (error) {
      CLIENT_LOG(3) << "connection to server failed (" << error << ").  retrying...";
      setup_connections();
   } else {
      recv_queue_ = std::make_shared<protocol::RecvQueue>(_tcp_socket);
      connected_ = true;
   }
}

void Client::setup_connections()
{
   connected_ = false;
   send_queue_ = protocol::SendQueue::Create(_tcp_socket);

   boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), server_port_);
   _tcp_socket.async_connect(endpoint, std::bind(&Client::handle_connect, this, std::placeholders::_1));
}

void Client::mainloop()
{
   process_messages();
   ProcessBrowserJobQueue();

   int game_time;
   float alpha;
   game_clock_->EstimateCurrentGameTime(game_time, alpha);

   Renderer::GetInstance().HandleResize();

   perfmon::SwitchToCounter("update lua");
   try {
      luabind::call_function<int>(radiant_["update"]);
   } catch (std::exception const& e) {
      CLIENT_LOG(3) << "error in client update: " << e.what();
      GetScriptHost()->ReportCStackThreadException(GetScriptHost()->GetCallbackThread(), e);
   }

   perfmon::SwitchToCounter("update browser frambuffer");
   browser_->UpdateBrowserFrambufferPtrs(
      (unsigned int*)Renderer::GetInstance().GetLastUiBuffer(), 
      (unsigned int*)Renderer::GetInstance().GetNextUiBuffer());

   perfmon::SwitchToCounter("flush http events");
   http_reactor_->FlushEvents();
   if (browser_) {
      perfmon::SwitchToCounter("browser poll");
      browser_->Work();
      auto cb = [](const csg::Region2 &rgn) {
         Renderer::GetInstance().UpdateUITexture(rgn);
      };
      perfmon::SwitchToCounter("update browser display");
      browser_->UpdateDisplay(cb);
   }

   InstallCurrentCursor();
   HilightMouseover();
   UpdateDebugCursor();

   // Fire the authoring traces *after* pumping the chrome message loop, since
   // we may create or modify authoring objects as a result of input events
   // or calls from the browser.
   perfmon::SwitchToCounter("fire traces");
   authoring_render_tracer_->Flush();

   Renderer::GetInstance().RenderOneFrame(game_time, alpha);

   if (send_queue_ && connected_) {
      perfmon::SwitchToCounter("send msgs");
      protocol::SendQueue::Flush(send_queue_);
   }

   // xxx: GC while waiting for vsync, or GC in another thread while rendering (ooh!)
   perfmon::SwitchToCounter("lua gc");
   platform::timer t(10);
   scriptHost_->GC(t);

   //Update the audio_manager with the current time
   audio::AudioManager &a = audio::AudioManager::GetInstance();
   a.UpdateAudio();
}

om::TerrainPtr Client::GetTerrain()
{
   om::EntityPtr rootObject = rootObject_.lock();   
   return rootObject ? rootObject->GetComponent<om::Terrain>() : nullptr;
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
      DISPATCH_MSG(ClearClientState);
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
}


void Client::EndUpdate(const proto::EndUpdate& msg)
{
   if (traceObjectRouter_) {
      traceObjectRouter_->CheckDeferredTraces();
   }

   if (!rootObject_.lock()) {
      auto rootEntity = GetStore().FetchObject<om::Entity>(1);
      if (rootEntity) {
         rootObject_ = rootEntity;
         Renderer::GetInstance().SetRootEntity(rootEntity);
         octtree_->SetRootEntity(rootEntity);
      }
   }

   // Only fire the remote store traces at sequence boundaries.  The
   // data isn't guaranteed to be in a consistent state between
   // boundaries.
   game_render_tracer_->Flush();
}

void Client::SetServerTick(const proto::SetServerTick& msg)
{
   int game_time = msg.now();
   game_clock_->Update(msg.now(), msg.interval(), msg.next_msg_time());
   Renderer::GetInstance().SetServerTick(game_time);
}

void Client::AllocObjects(const proto::AllocObjects& update)
{
   receiver_->ProcessAlloc(update);
}

void Client::UpdateObject(const proto::UpdateObject& update)
{
   receiver_->ProcessUpdate(update);
}

void Client::RemoveObjects(const proto::RemoveObjects& update)
{
   for (int id : update.objects()) {
      om::EntityPtr entity = GetEntity(id);

      if (entity) {
         auto render_entity = Renderer::GetInstance().GetRenderObject(entity);
         if (render_entity) {
            render_entity->Destroy();
         }
      }
   }
   receiver_->ProcessRemove(update);
}

void Client::UpdateDebugShapes(const proto::UpdateDebugShapes& msg)
{
   Renderer::GetInstance().DecodeDebugShapes(msg.shapelist());
}

void Client::ClearClientState(const proto::ClearClientState& msg)
{
   Shutdown();

   // remove all the input handlers except the one in the renderer.  this will
   // stop all tools as well as clearing out stale lua objects which had the
   // mouse captured (like the camera service)
   input_handlers_.clear();
   Renderer& renderer = Renderer::GetInstance();
   renderer.SetInputHandler([=](Input const& input) {
      OnInput(input);
   });


   Initialize();
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
      CLIENT_LOG(1) << "error dispatching input: " << e.what();
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
      if (input.mouse.up[0]) {
         CLIENT_LOG(5) << "updating selection...";
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
   perfmon::TimelineCounterGuard tcg("update selection") ;

   om::Selection s;
   Renderer::GetInstance().QuerySceneRay(mouse.x, mouse.y, s);

   if (s.HasEntities()) {
      auto entity = GetEntity(s.GetEntities().front());
      if (entity->GetComponent<om::Terrain>()) {
         CLIENT_LOG(3) << "clearing selection (clicked on terrain)";
         SelectEntity(nullptr);
      } else {
         CLIENT_LOG(3) << "selecting " << entity->GetObjectId();
         SelectEntity(entity);
      }
   } else {
      CLIENT_LOG(3) << "no entities!";
      SelectEntity(nullptr);
   }
}


void Client::SelectEntity(dm::ObjectId id)
{
   om::EntityPtr entity = GetEntity(id);
   if (entity) {
      SelectEntity(entity);
   }
}

void Client::SelectEntity(om::EntityPtr obj)
{
   om::EntityPtr selectedObject = selectedObject_.lock();
   RenderEntityPtr renderEntity;

   if (selectedObject != obj) {
      JSONNode selectionChanged(JSON_NODE);

      if (obj && obj->GetStore().GetStoreId() != GetStore().GetStoreId()) {
         CLIENT_LOG(3) << "ignoring selected object with non-client store id.";
         return;
      }

      if (selectedObject) {
         renderEntity = Renderer::GetInstance().GetRenderObject(selectedObject);
         if (renderEntity) {
            renderEntity->SetSelected(false);
         }
      }

      selectedObject_ = obj;
      if (obj) {
         CLIENT_LOG(3) << "Selected actor " << obj->GetObjectId();
         selected_trace_ = obj->TraceChanges("selection", dm::RENDER_TRACES)
                              ->OnDestroyed([=]() {
                                 SelectEntity(nullptr);
                              });

         renderEntity = Renderer::GetInstance().GetRenderObject(obj);
         if (renderEntity) {
            renderEntity->SetSelected(true);
         }
         std::string uri = obj->GetStoreAddress();
         selectionChanged.push_back(JSONNode("selected_entity", uri));
      } else {
         CLIENT_LOG(3) << "Cleared selected actor.";
      }

      http_reactor_->QueueEvent("radiant_selection_changed", selectionChanged);
   }
}

om::EntityPtr Client::GetEntity(dm::ObjectId id)
{
   dm::ObjectPtr obj = store_->FetchObject<dm::Object>(id);
   return (obj && obj->GetObjectType() == om::Entity::DmType) ? std::static_pointer_cast<om::Entity>(obj) : nullptr;
}

om::EntityRef Client::GetSelectedEntity()
{
   return selectedObject_;
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
   perfmon::TimelineCounterGuard tcg("hilight mouseover") ;

   om::Selection selection;
   auto &renderer = Renderer::GetInstance();
   csg::Point2 pt = renderer.GetMousePosition();

   renderer.QuerySceneRay(pt.x, pt.y, selection);

   om::EntityPtr selectedObject = selectedObject_.lock();
   for (const auto &e: hilightedObjects_) {
      om::EntityPtr entity = e.lock();
      if (entity && entity != selectedObject) {
         auto renderObject = renderer.GetRenderObject(entity);
         if (renderObject) {
            renderObject->SetSelected(false);
         }
      }
   }
   hilightedObjects_.clear();

   // hilight something
   if (selection.HasEntities()) {
      om::EntityPtr hilightEntity = GetEntity(selection.GetEntities().front());
      if (hilightEntity && hilightEntity != rootObject_.lock()) {
         RenderEntityPtr renderObject = renderer.GetRenderObject(hilightEntity);
         if (renderObject) {
            renderObject->SetSelected(true);
         }
         hilightedObjects_.push_back(hilightEntity);
      }
   }
}

void Client::UpdateDebugCursor()
{
   if (enable_debug_cursor_) {
      om::Selection selection;
      auto &renderer = Renderer::GetInstance();
      csg::Point2 pt = renderer.GetMousePosition();

      renderer.QuerySceneRay(pt.x, pt.y, selection);
      if (selection.HasBlock()) {
         json::Node args;
         csg::Point3 pt = selection.GetBlock() + csg::ToInt(selection.GetNormal());
         args.set("enabled", true);
         args.set("cursor", pt);
         core_reactor_->Call(rpc::Function("radiant:debug_navgrid", args));
         CLIENT_LOG(1) << "requesting debug shapes for nav grid tile " << csg::GetChunkIndex(pt, phys::TILE_SIZE);
      } else {
         json::Node args;
         args.set("enabled", false);
         core_reactor_->Call(rpc::Function("radiant:debug_navgrid", args));
      }
   }
}

Client::Cursor Client::LoadCursor(std::string const& path)
{
   Cursor cursor;

   std::string filename = res::ResourceManager2::GetInstance().ConvertToCanonicalPath(path, ".cur");
   auto i = cursors_.find(filename);
   if (i != cursors_.end()) {
      cursor = i->second;
   } else {
      std::shared_ptr<std::istream> is = res::ResourceManager2::GetInstance().OpenResource(filename);
      std::string buffer = io::read_contents(*is);
      std::string tempname = boost::filesystem::temp_directory_path().string() + boost::filesystem::unique_path().string();
      std::ofstream(tempname, std::ios::out | std::ios::binary).write(buffer.c_str(), buffer.size());

      HCURSOR hcursor = (HCURSOR)LoadImageA(GetModuleHandle(NULL), tempname.c_str(), IMAGE_CURSOR, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
      if (hcursor) {
         cursors_[filename] = cursor = Cursor(hcursor);
      }
      boost::filesystem::remove(tempname);
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
   JSONNode result = res::ResourceManager2::GetInstance().GetModules();
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
      CLIENT_LOG(3) << "error: got progress from deferred meant for rpc::HttpDeferredPtr! (logical error)";
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
   om::EntityPtr entity = authoringStore_->AllocObject<om::Entity>();   
   authoredEntities_[entity->GetObjectId()] = entity;
   CLIENT_LOG(7) << "created new authoring entity " << *entity;
   return entity;
}

om::EntityPtr Client::CreateAuthoringEntity(std::string const& uri)
{
   om::EntityPtr entity = CreateEmptyAuthoringEntity();
   om::Stonehearth::InitEntity(entity, uri, scriptHost_->GetInterpreter());
   CLIENT_LOG(7) << "created new empty authoring entity " << *entity;
   return entity;
}

void Client::DestroyAuthoringEntity(dm::ObjectId id)
{
   auto i = authoredEntities_.find(id);
   if (i != authoredEntities_.end()) {
      om::EntityPtr entity = i->second;

      CLIENT_LOG(7) << "destroying authoring entity " << *entity;
      entity->Destroy();
      if (entity) {
         auto render_entity = Renderer::GetInstance().GetRenderObject(entity);
         if (render_entity) {
            render_entity->Destroy();
         }
      }
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

XZRegionSelectorPtr Client::CreateXZRegionSelector()
{
   XZRegionSelectorPtr selector = std::make_shared<XZRegionSelector>(GetTerrain());
   xz_selectors_.push_back(selector);
   return selector;
}

void Client::DeactivateAllTools()
{
   stdutil::ForEachPrune<XZRegionSelector>(xz_selectors_, [=](XZRegionSelectorPtr s) {
      s->Deactivate();
   });
   xz_selectors_.clear();
}
   
void Client::RequestReload()
{
   core_reactor_->Call(rpc::Function("radiant:server:reload"));
}

