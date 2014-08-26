#include "pch.h"
#include <regex>
#include "build_number.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include "core/object_counter.h"
#include "core/config.h"
#include "radiant_file.h"
#include "radiant_exceptions.h"
#include "om/entity.h"
#include "om/components/mod_list.ridl.h"
#include "om/components/terrain.ridl.h"
#include "om/components/mod_list.ridl.h"
#include "om/error_browser/error_browser.h"
#include "om/selection.h"
#include "om/om_alloc.h"
#include "csg/lua/lua_csg.h"
#include "csg/random_number_generator.h"
#include "om/lua/lua_om.h"
#include "om/components/data_store.ridl.h"
#include "platform/utils.h"
#include "resources/manifest.h"
#include "resources/res_manager.h"
#include "om/stonehearth.h"
#include "horde3d/Source/Horde3DEngine/egModules.h"
#include "horde3d/Source/Horde3DEngine/egCom.h"
#include "horde3d/Bindings/C++/Horde3DUtils.h"
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
#include "lib/lua/physics/open.h"
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
#include "dm/store_trace.h"
#include "core/system.h"
#include "client/renderer/raycast_result.h"
#include "om/stonehearth.h"
#include "lib/perfmon/timer.h"
#include "renderer/render_node.h"

//  #include "GFx/AS3/AS3_Global.h"
#include "client.h"
#include "clock.h"
#include "renderer/renderer.h"
#include "libjson.h"

using namespace ::radiant;
using namespace ::radiant::client;
namespace fs = boost::filesystem;
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
   mouse_position_(csg::Point2::zero),
   perf_hud_shown_(false),
   connected_(false),
   game_clock_(nullptr),
   enable_debug_cursor_(false),
   flushAndLoad_(false),
   initialUpdate_(false),
   save_stress_test_(false),
   debug_track_object_lifetime_(false),
   loading_(false)
{
}

Client::~Client()
{
   Shutdown();
}

void Client::InitializeUI(std::string const& args)
{
   core::Config const& config = core::Config::GetInstance();
   std::string main_mod = config.Get<std::string>("game.main_mod", "stonehearth");
   
   std::string docroot;
   res::ResourceManager2& resource_manager = res::ResourceManager2::GetInstance();
   resource_manager.LookupManifest(main_mod, [&](const res::Manifest& manifest) {
      docroot = manifest.get<std::string>("ui.homepage", "about:");
   });
   if (!args.empty()) {
      if (docroot.find('?') == std::string::npos) {
         docroot += '?';
      }
      docroot += args;
   }
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
   int screen_width = renderer.GetWindowWidth();
   int screen_height = renderer.GetWindowHeight();

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

   if (config.Get("enable_flush_and_load", false)) {
      flushAndLoadDelay_ = config.Get("flush_and_load_delay", 1000);
      if (boost::filesystem::is_directory("mods/")) {
         fileWatcher_.addWatch(strutil::utf8_to_unicode("mods/"), [this](FW::WatchID watchid, const std::wstring& dir, const std::wstring& filename, FW::Action action) -> void {
            // FW::Action::Delete followed by FW::Action::Add is used to indcate that a file has been renamed.
            if (action == FW::Action::Modified || action == FW::Action::Add) {
               // We're getting a double-null terminated string, for some reason.  Converting to c_str fixes this.
               std::wstring washed(filename.c_str());
               std::wstring luaExt(L".lua");
               std::wstring jsonExt(L".json");
               if (boost::algorithm::ends_with(washed, luaExt) || boost::algorithm::ends_with(washed, jsonExt))
               {
                  InitiateFlushAndLoad();
               }
            }
         }, true);
      }
   }

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
      _commands[GLFW_KEY_F2] = [=]() { EnableDisableLifetimeTracking(); };
      _commands[GLFW_KEY_F3] = [=]() { core_reactor_->Call(rpc::Function("radiant:toggle_step_paths")); };
      _commands[GLFW_KEY_F4] = [=]() { core_reactor_->Call(rpc::Function("radiant:step_paths")); };
      _commands[GLFW_KEY_F5] = [=]() { RequestReload(); };
      _commands[GLFW_KEY_F6] = [=]() { SaveGame("hotkey_save", json::Node()); };
      _commands[GLFW_KEY_F7] = [=]() { LoadGame("hotkey_save"); };
      //_commands[GLFW_KEY_F8] = [=]() { EnableDisableSaveStressTest(); };
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
      try {
         // Expand aliases to get the proper trace route.  This lets us trace things like 'stonehearth:foo'
         uri = res::ResourceManager2::GetInstance().ConvertToCanonicalPath(uri, ".json");
      } catch (std::exception const&) {
         // Not an alias.  And that's OK!
      }
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
         std::string uri = "";
         json::Node n(params.get_node("track"));
         if (n.type() == JSON_STRING) {
            uri = n.get_internal_node().as_string();
         } else if (n.type() == JSON_NODE) {
            JSONNode items = n.get("items", JSONNode());
            csg::RandomNumberGenerator &rng = csg::RandomNumberGenerator::DefaultInstance();
            uint c = rng.GetInt<uint>(0, items.size() - 1);
            ASSERT(c < items.size());
            uri = items.at(c).as_string();
         }
         
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

   core_reactor_->AddRouteV("radiant:client:select_entity", [this](rpc::Function const& f) {
      json::Node params(json::Node(f.args).get_node(0));
      std::string uri = params.as<std::string>("");
      CLIENT_LOG(5) << "radiant:client:select_entity " << uri;
      om::EntityPtr entity = GetStore().FetchObject<om::Entity>(uri);
      if (!entity) {
         entity = GetAuthoringStore().FetchObject<om::Entity>(uri);
      }
      if (entity) {
         CLIENT_LOG(5) << " selecting " << *entity;
         SelectEntity(entity);
      }
   });

   core_reactor_->AddRouteV("radiant:client:save_game", [this](rpc::Function const& f) {
      json::Node saveid(json::Node(f.args).get_node(0));
      json::Node gameinfo(json::Node(f.args).get_node(1));
      gameinfo.set("version", PRODUCT_FILE_VERSION_STR);
      SaveGame(saveid.as<std::string>(), gameinfo);
   });

   core_reactor_->AddRoute("radiant:client:load_game", [this](rpc::Function const& f) {
      json::Node saveid(json::Node(f.args).get_node(0));
      return LoadGame(saveid.as<std::string>());
   });
   core_reactor_->AddRouteV("radiant:client:delete_save_game", [this](rpc::Function const& f) {
      json::Node saveid(json::Node(f.args).get_node(0));
      DeleteSaveGame(saveid.as<std::string>());
   });

   core_reactor_->AddRouteJ("radiant:client:get_save_games", [this](rpc::Function const& f) {
      json::Node games;
      fs::path savedir = core::Config::GetInstance().GetSaveDirectory();
      if (fs::is_directory(savedir)) {
         for (fs::directory_iterator end_dir_it, it(savedir); it != end_dir_it; ++it) {
            try {
               std::string name = it->path().filename().string();
               std::ifstream jsonfile((it->path() / "metadata.json").string());
               JSONNode metadata = libjson::parse(io::read_contents(jsonfile));
               json::Node entry;
               entry.set("screenshot", BUILD_STRING("/r/screenshot/" << name << "/screenshot.png"));
               entry.set("gameinfo", metadata);

               games.set(name, entry);
            } catch (std::exception const&) {
            }
         }
      }
      return games;
   });
   core_reactor_->AddRoute("radiant:client:get_perf_counters", [this](rpc::Function const& f) {
      return StartPerformanceCounterPush();
   });

};

rpc::ReactorDeferredPtr Client::StartPerformanceCounterPush()
{
   if (!perf_counter_deferred_) {
      perf_counter_deferred_ = std::make_shared<rpc::ReactorDeferred>("client perf counters");
   }
   return perf_counter_deferred_ ;
}

void Client::PushPerformanceCounters()
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

      GetScriptHost()->ComputeCounters(addCounter);

      perf_counter_deferred_->Notify(counters);
   }
}

void Client::EnableDisableLifetimeTracking()
{
#ifdef ENABLE_OBJECT_COUNTER
   debug_track_object_lifetime_ = !debug_track_object_lifetime_;
   if (!debug_track_object_lifetime_) {
      // If we're turning off, let's dump everything.
      CLIENT_LOG(1) << "-- Object lifetimes -----------------------------------";
      auto objectmap = core::ObjectCounterBase::GetObjects();

      std::map<int, std::pair<core::ObjectCounterBase*, std::type_index>> sortedMap;
      for (const auto& pair : objectmap) {
         sortedMap.insert(std::make_pair(pair.second.first, std::make_pair(pair.first, pair.second.second)));
      }

      for (const auto& pair : sortedMap) {
         void* address = (void*)pair.second.first;
         if (pair.second.second.name() == "class radiant::om::DataStore") {
            address = (void*)dynamic_cast<radiant::om::DataStore*>(pair.second.first);
            ASSERT(address != nullptr);
         }
         CLIENT_LOG(1) << "     Age:" << (platform::get_current_time_in_ms() - pair.first) << " Kind: " << pair.second.second.name() << " Address: " << pair.second.first;
      }
   }
   core::ObjectCounterBase::TrackObjectLifetime(debug_track_object_lifetime_);
#endif
}

void Client::InitiateFlushAndLoad()
{
   // Set the flush and load flag and set a timer.  We defer the actual flush and load until
   // the timer expires, both to ignore multiple callbacks from the file watcher and to make sure
   // we load after the *last* file in a "Save All" operation from an editor has made its way to
   // disk.
   flushAndLoad_ = true;
   flushAndLoadTimer_.set(flushAndLoadDelay_);
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
   ShutdownDataObjectTraces();
   ShutdownDataObjects();
}

void Client::InitializeDataObjects()
{
   scriptHost_.reset(new lua::ScriptHost("client", [this](int storeId) {
      return AllocateDatastore(storeId);
   }));
   store_.reset(new dm::Store(2, "game"));
   authoringStore_.reset(new dm::Store(3, "tmp"));

   om::RegisterObjectTypes(*store_);
   om::RegisterObjectTypes(*authoringStore_);

   // Keep track of when entities are allocated so we can wire up client side
   // lua components
   game_store_alloc_trace_ = store_->TraceStore("wire lua components")->OnAlloced([=](dm::ObjectPtr obj) {
      if (obj->GetObjectType() == om::DataStoreObjectType) {
         datastores_to_restore_.emplace_back(std::static_pointer_cast<om::DataStore>(obj));
      }
   });

}


void Client::InitializeDataObjectTraces()
{
   receiver_ = std::make_shared<dm::Receiver>(*store_);

   game_render_tracer_ = std::make_shared<dm::TracerBuffered>("client render", *store_);
   authoring_render_tracer_ = std::make_shared<dm::TracerBuffered>("client tmp render", *authoringStore_);
   game_object_model_traces_ = std::make_shared<dm::TracerSync>("client om");
   store_->AddTracer(game_render_tracer_, dm::RENDER_TRACES);
   store_->AddTracer(game_render_tracer_, dm::LUA_SYNC_TRACES);
   store_->AddTracer(game_render_tracer_, dm::LUA_ASYNC_TRACES);
   store_->AddTracer(game_render_tracer_, dm::RPC_TRACES);
   store_->AddTracer(game_object_model_traces_, dm::OBJECT_MODEL_TRACES);

   authoring_object_model_traces_ = std::make_shared<dm::TracerSync>("client om");
   authoringStore_->AddTracer(authoring_render_tracer_, dm::RENDER_TRACES);
   authoringStore_->AddTracer(authoring_render_tracer_, dm::LUA_SYNC_TRACES);
   authoringStore_->AddTracer(authoring_render_tracer_, dm::LUA_ASYNC_TRACES);
   authoringStore_->AddTracer(authoring_render_tracer_, dm::RPC_TRACES);
   authoringStore_->AddTracer(authoring_object_model_traces_, dm::OBJECT_MODEL_TRACES);
}

void Client::ShutdownDataObjectTraces()
{
   game_render_tracer_.reset();
   authoring_render_tracer_.reset();
   game_object_model_traces_.reset();
   authoring_object_model_traces_.reset();

   game_store_alloc_trace_.reset();
   lua_component_traces_.clear();
   datastores_to_restore_.clear();

   receiver_->Shutdown();
}

void Client::ShutdownDataObjects()
{
   datastoreMap_.clear();

   radiant_ = luabind::object();

   // the script host must go last, after all the luabind::objects spread out
   // in the system have been destroyed.
   scriptHost_.reset();

   authoringStore_.reset();
   receiver_.reset();
   store_.reset();
}

void Client::InitializeGameObjects()
{
   Renderer& renderer = Renderer::GetInstance();

   renderer.Initialize();

   octtree_.reset(new phys::OctTree(dm::RENDER_TRACES));

   luaModuleRouter_ = std::make_shared<rpc::LuaModuleRouter>(scriptHost_.get(), "client");
   luaObjectRouter_ = std::make_shared<rpc::LuaObjectRouter>(scriptHost_.get(), GetAuthoringStore());
   traceObjectRouter_ = std::make_shared<rpc::TraceObjectRouter>(*store_);
   traceAuthoredObjectRouter_ = std::make_shared<rpc::TraceObjectRouter>(*authoringStore_);

   core_reactor_->AddRouter(traceObjectRouter_);
   core_reactor_->AddRouter(traceAuthoredObjectRouter_);
   core_reactor_->AddRouter(luaModuleRouter_);
   core_reactor_->AddRouter(luaObjectRouter_);

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
   lua::phys::open(L, *octtree_);
   lua::res::open(L);
   lua::voxel::open(L);
   lua::rpc::open(L, core_reactor_);
   lua::analytics::open(L);
   lua::audio::open(L);
}

void Client::ShutdownGameObjects()
{
   Renderer::GetInstance().Shutdown();
   rootEntity_.reset();
   hilightedEntity_.reset();
   authoredEntities_.clear();
   
   game_clock_.reset();
   Horde3D::Modules::log().SetNotifyErrorCb(nullptr);
   scriptHost_->SetNotifyErrorCb(nullptr);

   perf_counter_deferred_ = nullptr;

   core_reactor_->RemoveRouter(luaModuleRouter_);
   core_reactor_->RemoveRouter(luaObjectRouter_);
   core_reactor_->RemoveRouter(traceObjectRouter_);
   core_reactor_->RemoveRouter(traceAuthoredObjectRouter_);

   luaModuleRouter_ = nullptr;
   luaObjectRouter_ = nullptr;
   traceObjectRouter_ = nullptr;
   traceAuthoredObjectRouter_ = nullptr;

   error_browser_.reset();
   octtree_.reset();
}

void Client::InitializeLuaObjects()
{
   hover_cursor_ = LoadCursor("stonehearth:cursors:hover");
   default_cursor_ = LoadCursor("stonehearth:cursors:default");
}

void Client::ShutdownLuaObjects()
{
   // keep the cursors around between save/load...
   localModList_ = nullptr;
   localRootEntity_ = nullptr;
}

void Client::run(int server_port)
{
   Renderer& renderer = Renderer::GetInstance();
   
   server_port_ = server_port;

   OneTimeIninitializtion();
   Initialize();
   CreateGame();

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
   PushPerformanceCounters();

   process_messages();
   ProcessBrowserJobQueue();

   CLIENT_LOG(5) << "entering client main loop";

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

   if (!loading_) {
      perfmon::SwitchToCounter("update browser frambuffer");
   }
   perfmon::SwitchToCounter("flush http events");
   http_reactor_->FlushEvents();

   if (browser_) {
      perfmon::SwitchToCounter("browser poll");
      browser_->Work();

      if (!loading_) {
         auto cb = [this](const csg::Region2 &rgn, const uint32* buff) {
            Renderer::GetInstance().UpdateUITexture(rgn, buff);
         };
         perfmon::SwitchToCounter("update browser display");
         browser_->UpdateDisplay(cb);
      }
   }

   InstallCurrentCursor();
   UpdateDebugCursor();

   // Fire the authoring traces *after* pumping the chrome message loop, since
   // we may create or modify authoring objects as a result of input events
   // or calls from the browser.
   perfmon::SwitchToCounter("fire traces");
   authoring_render_tracer_->Flush();

   // xxx: GC while waiting for vsync, or GC in another thread while rendering (ooh!)
   perfmon::SwitchToCounter("lua gc");
   CLIENT_LOG(7) << "running gc";
   platform::timer t(10);
   scriptHost_->GC(t);

   CLIENT_LOG(7) << "rendering one frame";
   Renderer::GetInstance().RenderOneFrame(game_time, alpha);

   if (send_queue_ && connected_) {
      perfmon::SwitchToCounter("send msgs");
      protocol::SendQueue::Flush(send_queue_);
   }


   //Update the audio_manager with the current time
   audio::AudioManager &a = audio::AudioManager::GetInstance();
   a.UpdateAudio();

   if (flushAndLoad_ && flushAndLoadTimer_.expired()) {
      flushAndLoad_ = false;
      SaveGame("filewatcher_save", json::Node());
      LoadGame("filewatcher_save");
   }

   if (save_stress_test_ && save_stress_test_timer_.expired()) {
      SaveGame("stress_test_save", json::Node());
      LoadGame("stress_test_save");
      save_stress_test_timer_.set(10000);
   }

   CLIENT_LOG(5) << "exiting client main loop";
}

om::TerrainPtr Client::GetTerrain()
{
   om::EntityPtr rootObject = rootEntity_.lock();   
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
      DISPATCH_MSG(UpdateObjectsInfo);
      DISPATCH_MSG(RemoveObjects);
      DISPATCH_MSG(UpdateDebugShapes);
      DISPATCH_MSG(PostCommandReply);
      DISPATCH_MSG(LoadGame);
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
   perfmon::TimelineCounterGuard tcg("end update");
   if (initialUpdate_) {
      if (load_progress_deferred_) {
         Renderer::GetInstance().SetLoading(false);
         loading_ = false;
         load_progress_deferred_->Resolve(JSONNode("progress", 100.0));
         load_progress_deferred_.reset();
      }
   }
   initialUpdate_ = false;

   // Constr
   RestoreDatastores();

   if (traceObjectRouter_) {
      traceObjectRouter_->CheckDeferredTraces();
   }

   if (!rootEntity_.lock()) {
      auto rootEntity = GetStore().FetchObject<om::Entity>(1);
      if (rootEntity) {
         rootEntity_ = rootEntity;
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


void Client::UpdateObjectsInfo(const proto::UpdateObjectsInfo& update)
{
   if (initialUpdate_) {
      lastLoadProgress_ = 0;
      networkUpdatesCount_ = 0;
      ReportLoadProgress();
   }
   networkUpdatesExpected_ = update.update_count() + update.remove_count();
}

void Client::UpdateObject(const proto::UpdateObject& update)
{
   receiver_->ProcessUpdate(update);
   if (initialUpdate_) {
      networkUpdatesCount_++;
      ReportLoadProgress();
   }
}

void Client::RemoveObjects(const proto::RemoveObjects& update)
{
   for (int id : update.objects()) {
      dm::ObjectPtr obj = store_->FetchObject<dm::Object>(id);
      if (obj && obj->GetObjectType() == om::EntityObjectType) {
         auto render_entity = Renderer::GetInstance().GetRenderEntity(std::static_pointer_cast<om::Entity>(obj));
         if (render_entity) {
            render_entity->Destroy();
         }
      }
   }
   receiver_->ProcessRemove(update, [this](dm::ObjectPtr obj) {
      if (obj->GetObjectType() == om::EntityObjectType) {
         std::static_pointer_cast<om::Entity>(obj)->Destroy();
      } else if (obj->GetObjectType() == om::DataStoreObjectType) {
         std::static_pointer_cast<om::DataStore>(obj)->DestroyController();
      }
   });
   if (initialUpdate_) {
      networkUpdatesCount_++;
      ReportLoadProgress();
   }
}

void Client::UpdateDebugShapes(const proto::UpdateDebugShapes& msg)
{
   Renderer::GetInstance().DecodeDebugShapes(msg.shapelist());
}

void Client::LoadGame(const proto::LoadGame& msg)
{
   CLIENT_LOG(2) << "load game";
   fs::path savedir = core::Config::GetInstance().GetSaveDirectory() / msg.game_id();
   LoadClientState(savedir);
   InitializeUI("state=load_finished");
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
   // Allow the client a huge amount of time to process the read queue (since we want to process
   // all of it in one go.)
   platform::timer t(50000);
   if (recv_queue_) {
      recv_queue_->Process<proto::Update>(std::bind(&Client::ProcessMessage, this, std::placeholders::_1), t);
   }
}

void Client::OnInput(Input const& input) {
   perfmon::SwitchToCounter("dispatching input");

   // If we're loading, we don't want the user to be able to click anything.
   if (loading_) {
      return;
   }
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
   mouse_position_.x = input.mouse.x;
   mouse_position_.y = input.mouse.y;

   browser_->OnInput(input);

   if (browser_->HasMouseFocus(input.mouse.x, input.mouse.y)) {
      return;
   }

   CallInputHandlers(input);
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

void Client::SelectEntity(om::EntityPtr entity)
{
   CLIENT_LOG(3) << "selecting " << entity;

   om::EntityPtr selectedEntity = selectedEntity_.lock();
   RenderEntityPtr renderEntity;

   if (selectedEntity != entity) {
      JSONNode selectionChanged(JSON_NODE);

      if (entity && entity->GetStore().GetStoreId() != GetStore().GetStoreId()) {
         CLIENT_LOG(3) << "ignoring selected object with non-client store id.";
         return;
      }

      if (selectedEntity) {
         renderEntity = Renderer::GetInstance().GetRenderEntity(selectedEntity);
         if (renderEntity) {
            renderEntity->SetSelected(false);
         }
      }

      selectedEntity_ = entity;
      if (entity) {
         CLIENT_LOG(3) << "selected entity " << *entity;
         selected_trace_ = entity->TraceChanges("selection", dm::RENDER_TRACES)
                              ->OnDestroyed([=]() {
                                 SelectEntity(nullptr);
                              });

         renderEntity = Renderer::GetInstance().CreateRenderEntity(RenderNode::GetUnparentedNode(), entity);
         if (renderEntity) {
            renderEntity->SetSelected(true);
         }
         std::string uri = entity->GetStoreAddress();
         selectionChanged.push_back(JSONNode("selected_entity", uri));
      } else {
         CLIENT_LOG(3) << "cleared selected entity.";
      }

      http_reactor_->QueueEvent("radiant_selection_changed", selectionChanged);
   } else {
      CLIENT_LOG(3) << "same entity.  bailing";
   }
}

#if 0
om::EntityPtr Client::GetEntity(dm::ObjectId id)
{
   dm::ObjectPtr obj = store_->FetchObject<dm::Object>(id);
   return (obj && obj->GetObjectType() == om::Entity::DmType) ? std::static_pointer_cast<om::Entity>(obj) : nullptr;
}
#endif

om::EntityRef Client::GetSelectedEntity()
{
   return selectedEntity_;
}

void Client::InstallCurrentCursor()
{
   if (browser_) {
      HCURSOR cursor;
      if (browser_->HasMouseFocus(mouse_position_.x, mouse_position_.y)) {
         cursor = uiCursor_;
      } else {
         if (!cursor_stack_.empty()) {
            cursor = cursor_stack_.back().second.get();
         } else if (!hilightedEntity_.expired()) {
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

void Client::HilightEntity(om::EntityPtr hilight)
{
   auto &renderer = Renderer::GetInstance();
   om::EntityPtr selectedEntity = selectedEntity_.lock();
   om::EntityPtr hilightedEntity = hilightedEntity_.lock();

   if (hilightedEntity && hilightedEntity != selectedEntity) {
      auto renderObject = renderer.GetRenderEntity(hilightedEntity);
      if (renderObject) {
         renderObject->SetSelected(false);
      }
   }
   hilightedEntity_.reset();

   if (hilight && hilight != rootEntity_.lock()) {
      RenderEntityPtr renderObject = renderer.GetRenderEntity(hilight);
      if (renderObject) {
         renderObject->SetSelected(true);
      }
      hilightedEntity_ = hilight;
   }
}

void Client::UpdateDebugCursor()
{
   if (enable_debug_cursor_) {   
      auto &renderer = Renderer::GetInstance();
      csg::Point2 pt = renderer.GetMousePosition();

      RaycastResult cast = renderer.QuerySceneRay(pt.x, pt.y, 0);
      if (cast.GetNumResults() > 0) {
         RaycastResult::Result r = cast.GetResult(0);
         json::Node args;
         csg::Point3 pt = r.brick + csg::ToInt(r.normal);
         om::EntityPtr selectedEntity = selectedEntity_.lock();
         args.set("enabled", true);
         args.set("cursor", pt);
         if (selectedEntity) {
            args.set("pawn", selectedEntity->GetStoreAddress());
         }
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
      // we used to use boost::filesystem routines to create a temporary path.  Unfortuantely, boost is pretty
      // paranoid about that sort of thing and tries to create a path that's cryptographically random.  This
      // ends up spinning up the crypto service, which does like 50- registry reads, creates additional temporary
      // files in AppData\Roaming\Microsoft\Crypto\RSA\{$GUID} and all sorts of crazy things.  It's just a cursor,
      // sheesh!  Furthemore, many of these things may fail is the app is not "properly installed", which
      // can cause problems on systems where we didn't even *do* and install (like if the user extracted a
      // zipfile).  So just pick something "random enough"
      boost::filesystem::path tmpdir = core::System::GetInstance().GetTempDirectory();
      std::string tempname = BUILD_STRING("cursor" << GetCurrentProcessId() << platform::get_current_time_in_ms());
      auto fulldir = tmpdir / tempname;
      std::ofstream(fulldir.string(), std::ios::out | std::ios::binary).write(buffer.c_str(), buffer.size());

      HCURSOR hcursor = (HCURSOR)LoadImageA(GetModuleHandle(NULL), fulldir.string().c_str(), IMAGE_CURSOR, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
      if (hcursor) {
         cursors_[filename] = cursor = Cursor(hcursor);
      }
      boost::filesystem::remove(fulldir);
   }
   return cursor;
}

Client::CursorStackId Client::InstallCursor(std::string const& name)
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
      int code;      
      std::smatch match;
      std::string localFilePath, content, mimetype;

      static const std::regex call_path_regex__("/r/call/?");
      static const std::regex screenshot_path_regex_("/r/screenshot/(.*)");

      if (std::regex_match(path, match, call_path_regex__)) {
         std::lock_guard<std::mutex> guard(browserJobQueueLock_);
         browserJobQueue_.emplace_back([=]() {
            CallHttpReactor(path, query, postdata, response);
         });
         return;
      }

      // file requests can be dispatched immediately.
      bool success = false;
      if (std::regex_match(path, match, screenshot_path_regex_)) {
         std::string localFilePath = (core::Config::GetInstance().GetSaveDirectory() / match[1].str()).string();
         success = http_reactor_->HttpGetFile(localFilePath, code, content, mimetype);
      } else {
         success = http_reactor_->HttpGetResource(path, code, content, mimetype);
      }
      if (success) {
         response->Resolve(rpc::HttpResponse(code, content, mimetype));
      } else {
         response->Reject(rpc::HttpError(code, content));
      }
   } catch (std::exception &e) {
      JSONNode fail;
      fail.push_back(JSONNode("error", e.what()));
      response->Reject(rpc::HttpError(500, fail.write()));
   }
}

void Client::CallHttpReactor(std::string const& path, const json::Node& query, std::string const& postdata, rpc::HttpDeferredPtr response)
{
   JSONNode node;
   int status = 404;
   rpc::ReactorDeferredPtr d = http_reactor_->Call(query, postdata);

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

om::DataStoreRef Client::AllocateDatastore(int storeId)
{
   om::DataStorePtr result;
   if (storeId == store_->GetStoreId()) {
      result = store_->AllocObject<om::DataStore>();   
   } else {
      result = authoringStore_->AllocObject<om::DataStore>();
      datastoreMap_[result->GetObjectId()] = result;
   }
   return result;
}

void Client::DestroyDatastore(dm::ObjectId id) {
   auto ds = datastoreMap_.find(id);
   if (ds != datastoreMap_.end()) {
      ds->second->DestroyController();
      datastoreMap_.erase(ds);
   }
}

om::EntityPtr Client::CreateAuthoringEntity(std::string const& uri)
{
   om::EntityPtr entity = authoringStore_->AllocObject<om::Entity>();   
   om::Stonehearth::InitEntity(entity, uri, scriptHost_->GetInterpreter());

   authoredEntities_[entity->GetObjectId()] = entity;

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
         auto render_entity = Renderer::GetInstance().GetRenderEntity(entity);
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

void Client::EnableDisableSaveStressTest()
{
   save_stress_test_ = !save_stress_test_;

   if (save_stress_test_) {
      save_stress_test_timer_.set(10000);
   }
}
   
void Client::RequestReload()
{
   core_reactor_->Call(rpc::Function("radiant:server:reload"));
}

void Client::SaveGame(std::string const& saveid, json::Node const& gameinfo)
{
   fs::path savedir = core::Config::GetInstance().GetSaveDirectory() / saveid;
   if (!fs::is_directory(savedir)) {
      fs::create_directories(savedir);
   } else {
      for (fs::directory_iterator end_dir_it, it(savedir); it != end_dir_it; ++it) {
         remove_all(it->path());
      }
   }
   SaveClientState(savedir);
   SaveClientMetadata(savedir, gameinfo);

   json::Node args;
   args.set("saveid", saveid);
   core_reactor_->Call(rpc::Function("radiant:server:save", args));
}

void Client::DeleteSaveGame(std::string const& saveid)
{
   fs::path savedir = core::Config::GetInstance().GetSaveDirectory() / saveid;
   if (fs::is_directory(savedir)) {
      remove_all(savedir);
   }
}

rpc::ReactorDeferredPtr Client::LoadGame(std::string const& saveid)
{
   fs::path savedir = core::Config::GetInstance().GetSaveDirectory() / saveid;
   if (fs::is_directory(savedir)) {
      loading_ = true;
      json::Node args;
      args.set("saveid", saveid);
      server_load_deferred_ = core_reactor_->Call(rpc::Function("radiant:server:load", args));
      
      load_progress_deferred_ = std::make_shared<rpc::ReactorDeferred>("load progress");
      server_load_deferred_->Progress([this] (const JSONNode n) {
         double server_progress = n.at("progress").as_float();
         load_progress_deferred_->Notify(JSONNode("progress", server_progress / 2.0));
      });
      Renderer::GetInstance().SetLoading(true);

      load_progress_deferred_->Progress([this] (const JSONNode n) {
         float progress = n.as_float();
         Renderer::GetInstance().UpdateLoadingProgress(progress / 100.0f);
      });
   }

   return load_progress_deferred_;
}

void Client::SaveClientMetadata(boost::filesystem::path const& savedir, json::Node const& gameinfo)
{
   h3dutScreenshot((savedir / "screenshot.png").string().c_str() );
   std::ofstream metadata((savedir / "metadata.json").string());
   metadata << gameinfo.write_formatted() << std::endl;
}

void Client::SaveClientState(boost::filesystem::path const& savedir)
{
   CLIENT_LOG(0) << "starting save.";
   std::string error;
   std::string filename = (savedir / "client_state.bin").string();
   if (!GetAuthoringStore().Save(filename, error)) {
      CLIENT_LOG(0) << "failed to save: " << error;
   }
   CLIENT_LOG(0) << "saved.";
}

void Client::LoadClientState(boost::filesystem::path const& savedir)
{
   CLIENT_LOG(0) << "starting loadin... " << savedir;

   // shutdown and initialize.  we need to initialize before creating the new streamers.  otherwise,
   // we won't have the new store for them.
   Shutdown();
   Initialize();

   std::string error;
   dm::ObjectMap objects;

   // Re-initialize the game
   dm::Store &store = GetAuthoringStore();
   std::string filename = (savedir / "client_state.bin").string();

   if (!store.Load(filename, error, objects, nullptr)) {
      CLIENT_LOG(0) << "failed to load: " << error;
   }

   // xxx: keep the trace around all the time to populate the authoredEntities_
   // array?  sounds good!!!
   std::vector<om::DataStorePtr> datastores;
   store.TraceStore("get entities")->OnAlloced([this, &datastores](dm::ObjectPtr obj) mutable {
      dm::ObjectId id = obj->GetObjectId();
      dm::ObjectType type = obj->GetObjectType();
      if (type == om::EntityObjectType) {
         authoredEntities_[id] = std::static_pointer_cast<om::Entity>(obj);
      } else if (obj->GetObjectType() == om::DataStoreObjectType) {
         om::DataStorePtr ds = std::static_pointer_cast<om::DataStore>(obj);
         datastores.emplace_back(ds);
         datastoreMap_[ds->GetObjectId()] = ds;
      }
   })->PushStoreState();   

   localRootEntity_ = GetAuthoringStore().FetchObject<om::Entity>(1);
   localModList_ = localRootEntity_->GetComponent<om::ModList>();

   CreateErrorBrowser();
   InitializeDataObjectTraces();

   radiant_ = scriptHost_->Require("radiant.client");
   scriptHost_->LoadGame(localModList_, authoredEntities_, datastores);  
   initialUpdate_ = true;


   CLIENT_LOG(0) << "done loadin...";
}

void Client::CreateGame()
{
   ASSERT(!localRootEntity_);
   ASSERT(!error_browser_);
   ASSERT(scriptHost_);

   InitializeUI("");

   localRootEntity_ = GetAuthoringStore().AllocObject<om::Entity>();
   ASSERT(localRootEntity_->GetObjectId() == 1);
   localModList_ = localRootEntity_->AddComponent<om::ModList>();

   CreateErrorBrowser();
   InitializeDataObjectTraces();

   radiant_ = scriptHost_->Require("radiant.client");
   scriptHost_->CreateGame(localModList_);
   initialUpdate_ = true;
}

void Client::CreateErrorBrowser()
{
   ASSERT(scriptHost_);

   error_browser_ = GetAuthoringStore().AllocObject<om::ErrorBrowser>();
   auto notifyErrorFn = [this](om::ErrorBrowser::Record const& r) {
      ASSERT(error_browser_);
      error_browser_->AddRecord(r);
   };
   scriptHost_->SetNotifyErrorCb(notifyErrorFn);
   Horde3D::Modules::log().SetNotifyErrorCb(notifyErrorFn);
}

void Client::ReportLoadProgress()
{
   ASSERT(initialUpdate_);

   if (networkUpdatesCount_) {
      int percent = (networkUpdatesCount_ * 100) / networkUpdatesExpected_;
      if (percent - lastLoadProgress_ > 10) {
         CLIENT_LOG(3) << " client load progress " << percent << "%...";

         if (load_progress_deferred_) {
            load_progress_deferred_->Notify(JSONNode("progress", 50.0 + (percent / 2.0)));
         }
         lastLoadProgress_ = percent;
      }
   }
}

/* 
 * -- Client::RestoreDatastores
 * 
 * Iterate through all the entities we created this frame and construct their
 * client-side lua components.
 */
void Client::RestoreDatastores()
{
   for (om::DataStorePtr d : datastores_to_restore_) {
      d->RestoreController(d);
   }
   datastores_to_restore_.clear();
}

csg::Point2 Client::GetMousePosition() const
{
   return mouse_position_;
}
