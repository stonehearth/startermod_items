#include "pch.h"
#include <regex>
#include "build_number.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
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
#include "lib/perfmon/sampling_profiler.h"
#include "lib/perfmon/report.h"
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
#include "renderer/perfhud/perfhud.h"
#include "renderer/perfhud/flame_graph_hud.h"

//  #include "GFx/AS3/AS3_Global.h"
#include "client.h"
#include "clock.h"
#include "renderer/renderer.h"
#include "libjson.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/URI.h"

using namespace ::radiant;
using namespace ::radiant::client;
namespace fs = boost::filesystem;
namespace proto = ::radiant::tesseract::protocol;

static const std::regex call_path_regex__("/r/call/?");
const char *HOTKEY_SAVE_KEY = "hotkey_save";
const char *SHIFT_F5_SAVE_KEY = "shift_f5_save";

static const std::string REDMINE_HOST = "104.197.61.23";
static const int REDMINE_PORT = 80;

static int POST_SYSINFO_DELAY_MS    = 15 * 1000;      // 15 seconds
static int POST_SYSINFO_INTERVAL_MS = 5 * 60 * 1000;  // every 5 minutes

DEFINE_SINGLETON(Client);

template<typename T> 
json::Node makeRendererConfigNode(RendererConfigEntry<T>& e) {
   json::Node n;

   n.set("value", e.value);
   n.set("allowed", e.allowed);

   return n;
}

static const std::string SAVE_TEMP_DIR("tmp_save");

Client::Client() :
   _tcp_socket(_io_service),
   store_(nullptr),
   authoringStore_(nullptr),
   uiCursor_(NULL),
   currentCursor_(NULL),
   next_input_id_(1),
   mouse_position_(csg::Point2::zero),
   connected_(false),
   game_clock_(nullptr),
   debug_cursor_mode_("none"),
   flushAndLoad_(false),
   initialUpdate_(false),
   save_stress_test_(false),
   debug_track_object_lifetime_(false),
   loading_(false),
   _lastSequenceNumber(0),
   _nextSysInfoPostTime(0),
   _currentUiScreen(InvalidScreen),
   _showDebugShapesMode(ShowDebugShapesMode::None),
   _asyncLoadPending(false)
{
   _nextSysInfoPostTime = platform::get_current_time_in_ms() + POST_SYSINFO_DELAY_MS;
   _allocDataStoreFn = [this]() {
      return AllocateDatastore();
   };
}

Client::~Client()
{
   Shutdown();
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
   browser_.reset(chromium::CreateBrowser(hwnd, "", renderer.GetScreenSize(), renderer.GetMinUiSize(), 1338));
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

   browserResizeGuard_ = renderer.OnScreenResize([this](csg::Rect2 const& r) {
      browser_->OnScreenResize(r);
   });

   InitializeUIScreen();

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
               if (boost::algorithm::ends_with(washed, luaExt) || boost::algorithm::ends_with(washed, jsonExt)) {
                  InitiateFlushAndLoad();
               }
            }
         }, true);
      }
   }

   if (config.Get("enable_debug_keys", false)) {
      _commands[GLFW_KEY_F1] = [this](KeyboardInput const& kb) {
         const char* mode = kb.shift ? "water_tight" : "navgrid";
         if (debug_cursor_mode_ != mode) {
            debug_cursor_mode_ = mode;
         } else {
            debug_cursor_mode_ = "none";
         }
         if (debug_cursor_mode_ == "none") {
            // toggle off
            json::Node args;
            args.set("mode", "none");
            core_reactor_->Call(rpc::Function("radiant:debug_navgrid", args));
         }
      };
      _commands[GLFW_KEY_F2] = [=](KeyboardInput const& kb) { EnableDisableLifetimeTracking(); };
      _commands[GLFW_KEY_F3] = [=](KeyboardInput const& kb) { core_reactor_->Call(rpc::Function("radiant:toggle_step_paths")); };
      _commands[GLFW_KEY_F4] = [=](KeyboardInput const& kb) { core_reactor_->Call(rpc::Function("radiant:step_paths")); };
      _commands[GLFW_KEY_F5] = [=](KeyboardInput const& kb) {
         if (kb.shift) {
            if (!_reloadSavePromise && !_reloadLoadPromise) {

               // Sweet.  No reload is in progress.  First save the game.  What that's done, load
               // the game we just saved.  The ->Always() blocks null out the promise pointers.
               // Always is always (ha!) called after Done or Reject.
               json::Node metadata;
               metadata.set("name", "Shift+F5 Save");
               _reloadSavePromise = SaveGame(SHIFT_F5_SAVE_KEY, metadata);

               _reloadSavePromise->Done([this](json::Node const&n) {
                                    ASSERT(!_reloadLoadPromise);

                                    _reloadLoadPromise = LoadGame(SHIFT_F5_SAVE_KEY);
                                    _reloadLoadPromise->Always([this]() {
                                       _reloadLoadPromise = nullptr;
                                    });
                                 });
               _reloadSavePromise->Always([this]() {
                                    _reloadSavePromise = nullptr;
                                 });
            }
         } else {
            ReloadBrowser();
         }
      };
      
      _commands[GLFW_KEY_F6] = [=](KeyboardInput const& kb) {
         json::Node metadata;
         metadata.set("name", "F6 Save");

         SaveGame(HOTKEY_SAVE_KEY, metadata);
      };

      _commands[GLFW_KEY_F7] = [=](KeyboardInput const& kb) { LoadGame(HOTKEY_SAVE_KEY); };
      //_commands[GLFW_KEY_F8] = [=](KeyboardInput const& kb) { EnableDisableSaveStressTest(); };
      _commands[GLFW_KEY_F9] = [=](KeyboardInput const& kb) { core_reactor_->Call(rpc::Function("radiant:toggle_debug_nodes")); };
      _commands[GLFW_KEY_F10] = [&renderer, this](KeyboardInput const& kb) {
         bool havePerfHud = _perfHud != nullptr;
         bool haveFlameGraphHud = _flameGraphHud != nullptr;
         Renderer& renderer = Renderer::GetInstance();

         _perfHud.reset();
         _flameGraphHud.reset();

         if (kb.shift) {
            if (!haveFlameGraphHud) {
               _flameGraphHud.reset(new FlameGraphHud(renderer));
            }
         } else {
            if (!havePerfHud) {
               _perfHud.reset(new PerfHud(renderer));
            }
         }
      };
      _commands[GLFW_KEY_F11] = [this, &renderer](KeyboardInput const& kb) {
         _showDebugShapesMode = (ShowDebugShapesMode)(((int)_showDebugShapesMode + 1) % ShowDebugShapesMode::TotalModes);
         if (_showDebugShapesMode == ShowDebugShapesMode::None) {
            CLIENT_LOG(0) << "hiding all debug shapes";
            renderer.SetShowDebugShapes(0);
         } else if (_showDebugShapesMode == ShowDebugShapesMode::All) {
            CLIENT_LOG(0) << "showing debug shapes for all entities";
            // Toggling this causes large memory leak in malloc (30 MB per toggle in a 25 tile world)
            renderer.SetShowDebugShapes(-1);
         } else {
            CLIENT_LOG(0) << "showing debug shapes for only the selected entity";
            om::EntityRef e = GetSelectedEntity();
            om::EntityPtr entity = e.lock();
            if (entity) {
               renderer.SetShowDebugShapes(entity->GetObjectId());
            }
         }
      };

      _commands[GLFW_KEY_PAUSE] = [](KeyboardInput const& kb) {
         // throw an exception that is not caught by Client::OnInput
         throw std::string("User hit crash key");
      };

      _commands[GLFW_KEY_KP_ENTER] = [=](KeyboardInput const& kb) { core_reactor_->Call(rpc::Function("radiant:write_lua_memory_profile")); };
      _commands[GLFW_KEY_KP_ADD] = [=](KeyboardInput const& kb) { core_reactor_->Call(rpc::Function("radiant:toggle_cpu_profile")); };
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
   core_reactor_->AddRoute("radiant:post_redmine_issues", [this](rpc::Function const& f) {
      rpc::ReactorDeferredPtr d = std::make_shared<rpc::ReactorDeferred>("filing bug");
      json::Node const& issues = f.args[0];
      json::Node const& options = f.args[1];
      bool screenshot = options.get<bool>("screenshot", true);

      json::Node result;
      if (PostRedmineIssues(issues, screenshot, result)) {
         d->Resolve(result);
      } else {
         d->Reject(result);
      }
      return d;
   });
   core_reactor_->AddRoute("radiant:get_modules", [this](rpc::Function const& f) {
      return GetModules(f);
   });
   core_reactor_->AddRoute("radiant:install_trace", [this](rpc::Function const& f) {
      json::Node args(f.args);
      std::string uri = args.get<std::string>(0, "");

      if (!GetStore().IsValidStoreAddress(uri)) {
         try {
            // Expand aliases to get the proper trace route.  This lets us trace things like 'stonehearth:foo'
            uri = res::ResourceManager2::GetInstance().ConvertToCanonicalPath(uri, ".json");
         } catch (std::exception const&) {
            // Not an alias.  And that's OK!
         }
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
         node.set("architecture", core::System::IsProcess64Bit() ? "x64" : "x32");
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
   core_reactor_->AddRouteV("radiant:send_performance_stats", [this](rpc::Function const& f) {
      ReportSysInfo();
   });

   core_reactor_->AddRoute("radiant:set_audio_config", [this](rpc::Function const& f) {
      rpc::ReactorDeferredPtr result = std::make_shared<rpc::ReactorDeferred>("radiant:set_player_volume");
      try {
         json::Node node(f.args);
         json::Node params = node.get_node(0);

         float bgm_volume = params.get<float>("bgm_volume");
         float efx_volume = params.get<float>("efx_volume");

         audio::AudioManager &a = audio::AudioManager::GetInstance();
         a.SetPlayerVolume(bgm_volume, efx_volume);

         result->ResolveWithMsg("success");

      } catch (std::exception const& e) {
         result->RejectWithMsg(BUILD_STRING("exception: " << e.what()));
      }
      return result;
   });

   core_reactor_->AddRoute("radiant:get_audio_config", [this](rpc::Function const& f) {
      rpc::ReactorDeferredPtr result = std::make_shared<rpc::ReactorDeferred>("radiant:get_audio_config");
      try {
         json::Node node;
         node.set("bgm_volume", audio::AudioManager::GetInstance().GetPlayerBgmVolume());
         node.set("efx_volume", audio::AudioManager::GetInstance().GetPlayerEfxVolume());

         result->Resolve(node);

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
         json::Node params = node.get_node(0);

         std::string sound_url = params.get<std::string>("track");
         
         audio::AudioManager &a = audio::AudioManager::GetInstance();
         int vol = params.get<int>("volume", audio::DEF_UI_SFX_VOL);
        
         a.PlaySound(sound_url, vol);
         
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

   core_reactor_->AddRouteJ("radiant:get_current_ui_screen", [this](rpc::Function const& f) {
      json::Node result;
      result.set("screen", GetCurrentUIScreen());
      return result;
   });

   core_reactor_->AddRouteV("radiant:show_game_screen", [this](rpc::Function const& f) {
      SetCurrentUIScreen(GameScreen, false);
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
         node.set("shadow_quality", makeRendererConfigNode(cfg.shadow_quality));
         node.set("max_lights", makeRendererConfigNode(cfg.max_lights));
         node.set("draw_distance", makeRendererConfigNode(cfg.draw_distance));
         node.set("gfx_card_renderer", Renderer::GetInstance().GetStats().gpu_renderer);
         node.set("gfx_card_driver", Renderer::GetInstance().GetStats().gl_version);
         node.set("use_high_quality", makeRendererConfigNode(cfg.use_high_quality));
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
         int flags = Renderer::APPLY_CONFIG_RENDERER;
         if (persistConfig) {
            flags |= Renderer::APPLY_CONFIG_PERSIST;
         }
         newCfg.use_shadows.value = params.get<bool>("shadows", oldCfg.use_shadows.value);
         newCfg.num_msaa_samples.value = params.get<int>("msaa", oldCfg.num_msaa_samples.value);
         newCfg.shadow_quality.value = params.get<int>("shadow_quality", oldCfg.shadow_quality.value);
         newCfg.max_lights.value = params.get<int>("max_lights", oldCfg.max_lights.value);
         newCfg.enable_fullscreen.value = params.get<bool>("fullscreen", oldCfg.enable_fullscreen.value);
         newCfg.enable_vsync.value = params.get<bool>("vsync", oldCfg.enable_vsync.value);
         newCfg.draw_distance.value = params.get<float>("draw_distance", oldCfg.draw_distance.value);
         newCfg.use_high_quality.value = params.get<bool>("use_high_quality", oldCfg.use_high_quality.value);
         newCfg.enable_ssao.value = params.get<bool>("enable_ssao", oldCfg.enable_ssao.value);
         
         Renderer::GetInstance().ApplyConfig(newCfg, flags);

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

   core_reactor_->AddRoute("radiant:client:get_portrait", [this](rpc::Function const& f) {
      rpc::ReactorDeferredPtr result = std::make_shared<rpc::ReactorDeferred>("radiant:client:get_portrait");

      Renderer::GetInstance().RequestPortrait([this, result](std::vector<unsigned char>& bytes) {
         json::Node res;
         res.set("bytes", libbase64::encode<std::string, char, unsigned char, false>(bytes.data(), bytes.size()));
         result->Resolve(res);
      });


      return result;
   });

   core_reactor_->AddRoute("radiant:client:save_game", [this](rpc::Function const& f) {
      json::Node saveid(json::Node(f.args).get_node(0));
      json::Node gameinfo(json::Node(f.args).get_node(1));
      gameinfo.set("version", PRODUCT_FILE_VERSION_STR);
      return SaveGame(saveid.as<std::string>(), gameinfo);
   });

   core_reactor_->AddRoute("radiant:client:load_game", [this](rpc::Function const& f) {
      json::Node saveid(json::Node(f.args).get_node(0));
      return LoadGame(saveid.as<std::string>());
   });
   core_reactor_->AddRouteV("radiant:client:load_game_async", [this](rpc::Function const& f) {
      json::Node saveid(json::Node(f.args).get_node(0));
      _asyncLoadName = saveid.as<std::string>();
      _asyncLoadPending = true;
   });
   core_reactor_->AddRouteV("radiant:client:delete_save_game", [this](rpc::Function const& f) {
      json::Node saveid(json::Node(f.args).get_node(0));
      DeleteSaveGame(saveid.as<std::string>());
   });

   core_reactor_->AddRouteJ("radiant:client:get_save_games", [this](rpc::Function const& f) {
      json::Node games;
      fs::path savedir = core::Config::GetInstance().GetSaveDirectory();

      CLIENT_LOG(3) << "listing saved games";

      if (!fs::is_directory(savedir)) {
         CLIENT_LOG(3) << "save game directory does not exist!";
         return games;
      }

      for (fs::directory_iterator end_dir_it, it(savedir); it != end_dir_it; ++it) {
         std::string name = it->path().filename().string();
         if (name == SAVE_TEMP_DIR) {
            CLIENT_LOG(3) << "ignoring temporary save directory: " << SAVE_TEMP_DIR;
            continue;
         }
         try {
            std::ifstream jsonfile((it->path() / "metadata.json").string());
            if (!jsonfile.good()) {
               CLIENT_LOG(3) << "ignoring directory \"" << name << "\" : no manifest.";
               continue;
            }
            JSONNode metadata = libjson::parse(io::read_contents(jsonfile));
            json::Node entry;
            entry.set("screenshot", BUILD_STRING("/r/screenshot/" << name << "/screenshot.png"));
            entry.set("gameinfo", metadata);
            games.set(name, entry);

            CLIENT_LOG(3) << "added save \"" << name << "\" to list.";
         } catch (std::exception const &e) {
            CLIENT_LOG(3) << "ignoring directory \"" << name << "\" :" << e.what();
         }
      }
      return games;
   });
   
   core_reactor_->AddRoute("radiant:client:get_perf_counters", [this](rpc::Function const& f) {
      return StartPerformanceCounterPush();
   });

   core_reactor_->AddRouteJ("radiant:get_config", [this](rpc::Function const& f) {
      json::Node args(f.args);
      std::string key = args.get<std::string>(0);
      JSONNode result;
      if (core::Config::GetInstance().Has(key)) {
         JSONNode node = core::Config::GetInstance().Get<JSONNode>(key);
         node.set_name(key);
         result.push_back(node);
      }
      return result;
   });

   core_reactor_->AddRouteJ("radiant:set_config", [this](rpc::Function const& f) {
      json::Node args(f.args);

      std::string key = args.get<std::string>(0);
      JSONNode value = args.get<JSONNode>(1);
      core::Config::GetInstance().Set<JSONNode>(key, value);

      json::Node result;
      result.set<std::string>("result", "OK");
      return result;
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
      core::ObjectCounterBase::ObjectMap const& objectmap = core::ObjectCounterBase::GetObjects();
      std::type_index datastoreTypeIndex(typeid(om::DataStore));

      perfmon::CounterValueType now = perfmon::Timer::GetCurrentCounterValueType();
      std::map<perfmon::CounterValueType, std::pair<core::ObjectCounterBase const*, std::string>> sortedMap;
      for (auto const& entry : objectmap) {
         std::type_index const& ti = entry.first;
         core::ObjectCounterBase::AllocationMap const& am = entry.second;

         for (auto const& alloc : am.allocs) {
            core::ObjectCounterBase const* obj = alloc.first; 
            perfmon::CounterValueType const& time = alloc.second;

            std::string name;
            if (ti == datastoreTypeIndex) {
               radiant::om::DataStore const* ds = dynamic_cast<radiant::om::DataStore const*>(obj);
               if (ds) {
                  std::string name = ds->GetControllerName();
                  if (!name.empty()) {
                     name += " controller";
                  }
               }
            }
            if (name.empty()) {
               name = am.typeName;
            }
            sortedMap[time] = std::make_pair(obj, name);
         }
      }

      for (const auto& entry : sortedMap) {
         perfmon::CounterValueType const& time = entry.first;
         core::ObjectCounterBase const* obj = entry.second.first;
         std::string const& name = entry.second.second;
         int age = perfmon::CounterToMilliseconds(now - time);

         CLIENT_LOG(1) << "     Age:" << age << " Kind: " << name << "   Address: " << obj;
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
   scriptHost_->Shutdown();
   store_->DisableAndClearTraces();
   ShutdownLuaObjects();
   ShutdownGameObjects();
   ShutdownDataObjectTraces();
   ShutdownDataObjects();
}

void Client::InitializeDataObjects()
{
   scriptHost_.reset(new lua::ScriptHost("client"));
   store_.reset(new dm::Store(2, "game"));
   authoringStore_.reset(new dm::Store(3, "tmp"));

   om::RegisterObjectTypes(*store_);
   om::RegisterObjectTypes(*authoringStore_);

   // Keep track of when entities are allocated so we can wire up client side
   // lua components
   game_store_alloc_trace_ = store_->TraceStore("wire lua components")->OnAlloced([=](dm::ObjectPtr const& obj) {
      dm::ObjectId id = obj->GetObjectId();
      if (obj->GetObjectType() == om::DataStoreObjectType) {
         datastores_to_restore_.emplace_back(std::make_pair(id, std::static_pointer_cast<om::DataStore>(obj)));
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

   CLIENT_LOG(1) << "user feedback is " << (analytics::GetCollectionStatus() ? "on" : "off");
   
   while (renderer.IsRunning()) {
      perfmon::BeginFrame(_perfHud != nullptr);
      perfmon::TimelineCounterGuard tcg("client run loop") ;
      mainloop();

#if defined(ENABLE_OBJECT_COUNTER)
      static int nextAuditTime = 0;
      int now = platform::get_current_time_in_ms();
      if (nextAuditTime == 0 || nextAuditTime < now) {
         if (receiver_) {
            receiver_->Audit();
         }
         nextAuditTime = now + 5000;
      }
#endif
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
      recv_queue_ = std::make_shared<protocol::RecvQueue>(_tcp_socket, "client");
      connected_ = true;
   }
}

void Client::setup_connections()
{
   connected_ = false;
   send_queue_ = protocol::SendQueue::Create(_tcp_socket, "client");

   boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), server_port_);
   _tcp_socket.async_connect(endpoint, std::bind(&Client::handle_connect, this, std::placeholders::_1));
}

void Client::mainloop()
{
   ASSERT(browser_ != nullptr);

   if (_asyncLoadPending) {
      _asyncLoadPending = false;
      LoadGame(_asyncLoadName);
   }

   PushPerformanceCounters();
   process_messages();

   CLIENT_LOG(5) << "entering client main loop";

   Renderer::GetInstance().HandleResize();

   perfmon::SwitchToCounter("browser queue");
   ProcessBrowserJobQueue();

   if (!loading_) {
      perfmon::SwitchToCounter("update lua");
      try {
         radiant_["update"]();
      } catch (std::exception const& e) {
         CLIENT_LOG(3) << "error in client update: " << e.what();
         GetScriptHost()->ReportCStackThreadException(GetScriptHost()->GetCallbackThread(), e);
      }
   }

   perfmon::SwitchToCounter("flush http events");
   http_reactor_->FlushEvents();

   perfmon::SwitchToCounter("browser poll");
   browser_->Work();

   if (!loading_) {
      auto cb = [this](csg::Point2 const& size, csg::Region2 const &rgn, const uint32* buff) {
         Renderer::GetInstance().UpdateUITexture(size, rgn, buff);
      };
      perfmon::SwitchToCounter("update browser display");
      browser_->UpdateDisplay(cb);

      // Fire the authoring traces *after* pumping the chrome message loop, since
      // we may create or modify authoring objects as a result of input events
      // or calls from the browser.
      perfmon::SwitchToCounter("fire traces");
      authoring_render_tracer_->Flush();
   }

   perfmon::SwitchToCounter("update cursor");
   InstallCurrentCursor();
   UpdateDebugCursor();

   // xxx: GC while waiting for vsync, or GC in another thread while rendering (ooh!)
   perfmon::SwitchToCounter("lua gc");
   CLIENT_LOG(7) << "running gc";
   platform::timer t(10);
   scriptHost_->GC(t);


   CLIENT_LOG(7) << "rendering one frame";
   int game_time;
   float alpha;
   game_clock_->EstimateCurrentGameTime(game_time, alpha);
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
   
   int now = platform::get_current_time_in_ms();
   if (_nextSysInfoPostTime < now) {
      ReportSysInfo();
      _nextSysInfoPostTime = now + POST_SYSINFO_INTERVAL_MS;
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
   default:
      ASSERT(false);
   }

   return true;
}

void Client::PostCommandReply(const proto::PostCommandReply& msg)
{
   try {
      protobuf_router_->OnPostCommandReply(msg);
   } catch (std::exception const& e) {
      CLIENT_LOG(1) << "got exception handling server reply: " << e.what();
   }
}

void Client::BeginUpdate(const proto::BeginUpdate& msg)
{
   _lastSequenceNumber = msg.sequence_number();
}


void Client::EndUpdate(const proto::EndUpdate& msg)
{
   perfmon::TimelineCounterGuard tcg("end update");

   // If we're still in limbo, bail.
   if (!receiver_) {
      CLIENT_LOG(3) << "ignoring EndUpdate. still in limbo while loading.";
      return;
   }

   // Acknowledge that we've finished processing the update.
   proto::Request ack;
   ack.set_type(proto::Request::FinishedUpdate);   
   proto::FinishedUpdate* r = ack.MutableExtension(proto::FinishedUpdate::extension);
   r->set_sequence_number(_lastSequenceNumber);
   send_queue_->Push(ack);

   if (initialUpdate_) {
      if (load_progress_deferred_) {
         if (loadError_.empty()) {
            SetCurrentUIScreen(GameScreen);
            load_progress_deferred_->Resolve(JSONNode("progress", 100.0));
         } else {
            SetCurrentUIScreen(TitleScreen);
            load_progress_deferred_->RejectWithMsg(loadError_);
         }
         loading_ = false;
         loadError_.clear();
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
         Renderer::GetInstance().ConstructAllRenderEntities();
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
   if (!receiver_) {
      CLIENT_LOG(3) << "ignoring SetServerTick. still in limbo while loading.";
      return;
   }

   int game_time = msg.now();
   game_clock_->Update(msg.now(), msg.interval(), msg.next_msg_time());
   Renderer::GetInstance().SetServerTick(game_time);
}

void Client::AllocObjects(const proto::AllocObjects& update)
{
   // If we're still in limbo, bail.
   if (!receiver_) {
      CLIENT_LOG(3) << "ignoring AllocObjects. still in limbo while loading.";
      return;
   }
   receiver_->ProcessAlloc(update);
}


void Client::UpdateObjectsInfo(const proto::UpdateObjectsInfo& update)
{
   if (initialUpdate_) {
      lastLoadProgress_ = 0;
      networkUpdatesCount_ = 0;
      ReportLoadProgress();

      CLIENT_LOG(3) << "in UpdateObjectsInfo. expecting " << update.update_count() << " updates and " << update.remove_count() << " deletes";
      networkUpdatesExpected_ = update.update_count() + update.remove_count();
   }
}

void Client::UpdateObject(const proto::UpdateObject& update)
{
   if (!receiver_) {
      CLIENT_LOG(3) << "ignoring UpdateObject. still in limbo while loading.";
      return;
   }

   receiver_->ProcessUpdate(update);
   if (initialUpdate_) {
      networkUpdatesCount_++;
      ReportLoadProgress();
   }
}

void Client::RemoveObjects(const proto::RemoveObjects& update)
{
   if (!receiver_) {
      CLIENT_LOG(3) << "ignoring RemoveObjects. still in limbo while loading.";
      return;
   }

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
         // Don't call destroy.  We don't own it!
         std::static_pointer_cast<om::DataStore>(obj)->CallLuaDestructor();
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
         i->second(input.keyboard);
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
         ++i;
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
   if (_showDebugShapesMode == ShowDebugShapesMode::Selected) {
      Renderer& renderer = Renderer::GetInstance();
      om::EntityPtr s = selectedEntity_.lock();
      if (s) {
         renderer.SetShowDebugShapes(s->GetObjectId());
      } else {
         renderer.SetShowDebugShapes(0);
      }
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

         SetClassLongPtr(hwnd, GCLP_HCURSOR, static_cast<LONG_PTR>(reinterpret_cast<LONG_PTR>(cursor)));
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
   if (debug_cursor_mode_ != "none") {
      auto &renderer = Renderer::GetInstance();
      csg::Point2 pt = renderer.GetMousePosition();

      RaycastResult cast = renderer.QuerySceneRay(pt.x, pt.y, 0);
      if (cast.GetNumResults() > 0) {
         RaycastResult::Result r = cast.GetResult(0);
         json::Node args;
         csg::Point3 pt = csg::ToInt(r.brick) + csg::ToInt(r.normal);
         om::EntityPtr selectedEntity = selectedEntity_.lock();
         args.set("enabled", true);
         args.set("mode", debug_cursor_mode_);
         args.set("cursor", pt);
         if (selectedEntity) {
            args.set("pawn", selectedEntity->GetStoreAddress());
         }
         core_reactor_->Call(rpc::Function("radiant:debug_navgrid", args));
         CLIENT_LOG(5) << "requesting debug shapes for nav grid tile " << csg::GetChunkIndex<phys::TILE_SIZE>(pt);
      } else {
         json::Node args;
         args.set("mode", "none");
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
   for (auto i = cursor_stack_.begin(); i != cursor_stack_.end(); ++i) {
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
      static const std::regex saved_object_path_regex_("/r/saved_objects/(.*)");

      if (std::regex_match(path, match, call_path_regex__)) {
         std::lock_guard<std::mutex> guard(browserJobQueueLock_);
         if (!loading_) {
            browserJobQueue_.emplace_back([=]() {
               CallHttpReactor(path, query, postdata, response);
            });
         }
         return;
      }

      // file requests can be dispatched immediately.
      bool success = false;
      if (std::regex_match(path, match, screenshot_path_regex_)) {
         std::string localFilePath = (core::Config::GetInstance().GetSaveDirectory() / match[1].str()).string();
         success = http_reactor_->HttpGetFile(localFilePath, code, content, mimetype);
      } else if (std::regex_match(path, match, saved_object_path_regex_)) {
         std::string localFilePath = (core::System::GetInstance().GetTempDirectory() / "saved_objects" / match[1].str()).string();
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

om::DataStorePtr Client::AllocateDatastore()
{
   return authoringStore_->AllocObject<om::DataStore>();
}

om::EntityPtr Client::CreateAuthoringEntity(std::string const& uri)
{
   om::EntityPtr entity = authoringStore_->AllocObject<om::Entity>();   
   om::Stonehearth::InitEntity(entity, uri.c_str(), scriptHost_->GetInterpreter());

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
   std::lock_guard<std::mutex> guard(browserJobQueueLock_);
   perfmon::TimelineCounterGuard tcg("process job queue");

   // Pump the browser queue until we get into the loading state,
   // then bail.  We release the lock before calling the callback
   // so new browser events can come in and to avoid deadlocks in
   // case resolving a deferred somehow, magically, ends up trying
   // to stick something else on the back of our queue, too.
   //
   while (!loading_ && !browserJobQueue_.empty()) {
      std::function<void()> cb = browserJobQueue_.front();
      browserJobQueue_.pop_front();
      browserJobQueueLock_.unlock();
      try {
         cb();
      } catch (std::exception const&e) {
         CLIENT_LOG(0) << "error in browser job callback: " << e.what();
      }
      browserJobQueueLock_.lock();
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
   
void Client::ReloadBrowser()
{
   std::lock_guard<std::mutex> guard(browserJobQueueLock_);

   std::string uri = BUILD_STRING(_uiDocroot << "?current_screen=" << GetCurrentUIScreen());
   browserJobQueue_.clear();
   browser_->Navigate(uri);
}

void Client::RequestReload()
{
   core_reactor_->Call(rpc::Function("radiant:server:reload"));
}

rpc::ReactorDeferredPtr Client::SaveGame(std::string const& saveid, json::Node const& gameinfo)
{
   rpc::ReactorDeferredPtr d = std::make_shared<rpc::ReactorDeferred>("client save");
   if (saveid.empty()) {
      CLIENT_LOG(0) << "ignoring save request: saveid is empty";
      d->RejectWithMsg("stonehearth:no_saveid");
      return d;
   }

   if (client_save_deferred_) {
      CLIENT_LOG(0) << "ignoring save request: request is already in flight";
      d->RejectWithMsg("stonehearth:save_in_progress");
      return d;
   }

   ASSERT(!server_save_deferred_);

   fs::path tmpdir  = core::Config::GetInstance().GetSaveDirectory() / SAVE_TEMP_DIR;
   if (fs::is_directory(tmpdir)) {
      remove_all(tmpdir);
   }
   fs::create_directories(tmpdir);

   SaveClientScreenShot(tmpdir);
   SaveClientState(tmpdir);

   json::Node args;
   args.set("saveid", SAVE_TEMP_DIR);

   client_save_deferred_ = d;
   server_save_deferred_ = core_reactor_->Call(rpc::Function("radiant:server:save", args));

   server_save_deferred_->Done([this, saveid, tmpdir, gameinfo](JSONNode const&n) {
      // Wait until both the client and server state have been saved to write
      // the client metadata.  This way we won't leave an incomplete file if we
      // (for example) crash while saving the server state.
      SaveClientMetadata(tmpdir, gameinfo);

      // finally. move the save game to the correct direcory
      fs::path savedir = core::Config::GetInstance().GetSaveDirectory() / saveid;
      DeleteSaveGame(saveid);
      rename(tmpdir, savedir);
      CLIENT_LOG(1) << "moving " << tmpdir.string() << " to " << savedir;

      client_save_deferred_->Resolve(n);
   });
   server_save_deferred_->Fail([this, tmpdir, gameinfo](JSONNode const&n) {
      client_save_deferred_->Reject(n);
   });

   server_save_deferred_->Always([this, tmpdir] {
      CLIENT_LOG(1) << "clearing save deferred pointers";
      client_save_deferred_ = nullptr;
      server_save_deferred_ = nullptr;
      remove_all(tmpdir);
   });

   return client_save_deferred_;
}

void Client::DeleteSaveGame(std::string const& saveid)
{
   ASSERT(!saveid.empty());

   fs::path savedir = core::Config::GetInstance().GetSaveDirectory() / saveid;
   if (fs::is_directory(savedir)) {
      remove_all(savedir);
   }
}

rpc::ReactorDeferredPtr Client::LoadGame(std::string const& saveid)
{
   rpc::ReactorDeferredPtr deferred = std::make_shared<rpc::ReactorDeferred>("load progress");

   if (load_progress_deferred_ != nullptr) {
      deferred->RejectWithMsg("ignoring load request when already loading");
      return deferred;
   }

   fs::path savedir = core::Config::GetInstance().GetSaveDirectory() / saveid;
   if (!fs::is_directory(savedir)) {
      deferred->RejectWithMsg(BUILD_STRING("save " << saveid << " does not exist"));
      return deferred;
   }

   {
      std::ifstream jsonfile((savedir / "metadata.json").string());

      CLIENT_LOG(0) << "loading save \"" << saveid << "\":";
      for (std::string line : strutil::split(io::read_contents(jsonfile), "\n")) {
         CLIENT_LOG(0) << line;
      }
   }

   loading_ = true;

   // Navigate to the empty page--this effectively shuts down the UI, removes all polling
   // and listeners to server-side stuff, so that we don't spam logs or consume unnecessary
   // cycles.
   core::Config const& config = core::Config::GetInstance();
   std::string main_mod = config.Get<std::string>("game.main_mod", "stonehearth");
   res::ResourceManager2& resource_manager = res::ResourceManager2::GetInstance();
   std::string emptyUrl;
   resource_manager.LookupManifest(main_mod, [&](const res::Manifest& manifest) {
      emptyUrl = manifest.get<std::string>("ui.empty", "about:");
   });
   browser_->Navigate(emptyUrl);
   
   loadError_.clear();
   load_progress_deferred_ = deferred;
   ASSERT(server_load_deferred_ == nullptr);

   json::Node args;
   args.set("saveid", saveid);
   server_load_deferred_ = core_reactor_->Call(rpc::Function("radiant:server:load", args));
   server_load_deferred_->Progress([this] (JSONNode const& n) {
      float progress = static_cast<float>(n.at("progress").as_float());

      CLIENT_LOG(1) << "server reported load progress of " << progress << "%";

      progress /= 2;
      load_progress_deferred_->Notify(JSONNode("progress", progress));
      Renderer::GetInstance().UpdateLoadingProgress(progress / 100.0f);
   });
   server_load_deferred_->Done([this, saveid] (JSONNode const& n) {
      fs::path savedir = core::Config::GetInstance().GetSaveDirectory() / saveid;
      CLIENT_LOG(0) << "server load finished.  loading client state.";
      LoadClientState(savedir);
   });
   server_load_deferred_->Fail([this, saveid] (JSONNode const& n) {
      CLIENT_LOG(0) << "server load failed.  returning to title screen.";
      CreateGame();
      loadError_ = n.at("error").as_string();
   });
   server_load_deferred_->Always([this] {
      server_load_deferred_.reset();
   });

   // We just navigated to the empty screen, so don't reload the UI (or we'll spam the client).
   SetCurrentUIScreen(LoadingScreen, false);
   Shutdown();
   Initialize();

   return load_progress_deferred_;
}

void Client::SaveClientScreenShot(boost::filesystem::path const& savedir)
{
   auto&r = Renderer::GetInstance();
   r.RenderOneFrame(r.GetLastRenderTime(), r.GetLastRenderAlpha(), true);
   h3dutScreenshot((savedir / "screenshot.png").string().c_str(), r.GetScreenshotTexture());
}

void Client::SaveClientMetadata(boost::filesystem::path const& savedir, json::Node const& gameinfo)
{
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
   CLIENT_LOG(0) << "starting loading... " << savedir;

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

   CLIENT_LOG(1) << "restoring datastores...";
   store.TraceStore("get entities")->OnAlloced([this, &datastores](dm::ObjectPtr const& obj) mutable {
      dm::ObjectId id = obj->GetObjectId();
      dm::ObjectType type = obj->GetObjectType();
      if (type == om::EntityObjectType) {
         authoredEntities_[id] = std::static_pointer_cast<om::Entity>(obj);
      } else if (obj->GetObjectType() == om::DataStoreObjectType) {
         om::DataStorePtr ds = std::static_pointer_cast<om::DataStore>(obj);
         dm::ObjectId id = ds->GetObjectId();
         datastores.emplace_back(ds);
      }
   })->PushStoreState();   

   CLIENT_LOG(1) << "recreating data objects...";

   localRootEntity_ = GetAuthoringStore().FetchObject<om::Entity>(1);
   localModList_ = localRootEntity_->GetComponent<om::ModList>();

   CLIENT_LOG(1) << "creating error browser...";
   CreateErrorBrowser();
   CLIENT_LOG(1) << "initializing data object traces...";
   InitializeDataObjectTraces();

   CLIENT_LOG(1) << "requiring radiant.client...";
   radiant_ = scriptHost_->Require("radiant.client");
   CLIENT_LOG(1) << "loading script host...";
   scriptHost_->LoadGame(localModList_, _allocDataStoreFn, authoredEntities_, datastores);  
   initialUpdate_ = true;

   platform::SysInfo::LogMemoryStatistics("Finished Loading Client", 0);
}

void Client::CreateGame()
{
   ASSERT(!localRootEntity_);
   ASSERT(!error_browser_);
   ASSERT(scriptHost_);

   localRootEntity_ = GetAuthoringStore().AllocObject<om::Entity>();
   ASSERT(localRootEntity_->GetObjectId() == 1);
   localModList_ = localRootEntity_->AddComponent<om::ModList>();

   CreateErrorBrowser();
   InitializeDataObjectTraces();

   radiant_ = scriptHost_->Require("radiant.client");
   scriptHost_->CreateGame(localModList_, _allocDataStoreFn);
   initialUpdate_ = true;
}

void Client::InitializeUIScreen()
{
   core::Config const& config = core::Config::GetInstance();
   std::string main_mod = config.Get<std::string>("game.main_mod", "stonehearth");

   bool skipTitle;
   res::ResourceManager2& resource_manager = res::ResourceManager2::GetInstance();
   resource_manager.LookupManifest(main_mod, [&](const res::Manifest& manifest) {
      _uiDocroot = manifest.get<std::string>("ui.homepage", "about:");
      skipTitle = manifest.get<bool>("ui.skip_title", false);
   });

   if (!skipTitle) {
      SetCurrentUIScreen(TitleScreen);
   } else {
      SetCurrentUIScreen(GameScreen);
   }
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

         if (load_progress_deferred_) {
            float progress = 50.0f + (percent / 2.0f);
            CLIENT_LOG(1) << "reporting client load progress " << progress << "%...";
            load_progress_deferred_->Notify(JSONNode("progress", progress));
            Renderer::GetInstance().UpdateLoadingProgress(progress / 100.0f);
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
   // Two passes: First create all the controllers for the datastores we just
   // created
   CLIENT_LOG(7) << "restoring datastores controllers";
   for (auto const& entry : datastores_to_restore_) {
      om::DataStorePtr ds = entry.second.lock();
      if (ds) {
         // Make the controller as cpp managed since the server controls the lifetime.
         ds->RestoreController(ds, om::DataStore::IS_CPP_MANAGED);
      } else {
         CLIENT_LOG(7) << "datastore id:" << entry.first << " did not surive the journey";
      }
   }
   CLIENT_LOG(7) << "finished restoring datastores controllers";

   // Now run through all the tables on those datastores and convert the
   // pointers-to-datastore to pointers-to-controllers
   CLIENT_LOG(7) << "restoring datastores controller data";
   for (auto const& entry : datastores_to_restore_) {
      om::DataStorePtr ds = entry.second.lock();
      if (ds) {
         ds->RestoreControllerData();
      } else {
         CLIENT_LOG(7) << "datastore id:" << entry.first << " did not surive the journey";
      }
   }

   CLIENT_LOG(7) << "removing keep alive references to datastores";
   for (auto const& entry : datastores_to_restore_) {
      om::DataStorePtr ds = entry.second.lock();
      if (ds) {
         ds->RemoveKeepAliveReferences();
      } else {
         CLIENT_LOG(7) << "datastore id:" << entry.first << " did not surive the journey";
      }
   }
   CLIENT_LOG(7) << "finished restoring datastores controller data";

   datastores_to_restore_.clear();
}

csg::Point2 Client::GetMousePosition() const
{
   return mouse_position_;
}

void Client::ReportSysInfo()
{
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
      node.set("OsArch", core::System::IsPlatform64Bit() ? "x64" : "x32");

      CLIENT_LOG(3) << "reporting sysinfo";

      // xxx, parse GAME_DEMOGRAPHICS_URL into domain and path, in postdata
      analytics::PostData post_data(node, REPORT_SYSINFO_URI,  "");
      post_data.Send();
   } catch (std::exception const& e) {
      CLIENT_LOG(1) << "failed to update sysinfo:" << e.what();
   }
}

const char* Client::GetCurrentUIScreen() const
{
   const char *screens[] = {
      "title_screen",
      "game_screen",
      "loading_screen",
   };
   ASSERT(_currentUiScreen >= 0 && _currentUiScreen < ARRAY_SIZE(screens));

   return screens[_currentUiScreen];
}

void Client::SetCurrentUIScreen(UIScreen screen, bool reloadRequested)
{
   if (screen == _currentUiScreen) {
      return;
   }

   _currentUiScreen = screen;
   Renderer& r = Renderer::GetInstance();
   r.SetLoading(screen == LoadingScreen);
   r.SetDrawWorld(screen == GameScreen);
   if (scriptHost_) {
      scriptHost_->Trigger("radiant:client:ui_screen_changed");
   }

   if (reloadRequested) {
      ReloadBrowser();
   }
}

bool Client::PostRedmineIssues(json::Node issues, bool takeScreenshot, json::Node& result)
{
   std::string apiKey = issues.get<std::string>("key", "");
   std::string token, payload;
   json::Node upload;
   json::Node uploadsArray(JSON_ARRAY);

   if (takeScreenshot) {
      fs::path sspath = core::Config::GetInstance().GetSaveDirectory() / SAVE_TEMP_DIR / "bug_screenshot.png";
      fs::remove_all(sspath);
      h3dutScreenshot(sspath.string().c_str(), 0);
      payload = io::read_contents(std::ifstream(sspath.string(), std::ifstream::in | std::ifstream::binary));
      //fs::remove_all(sspath);

      if (!PostRedmineUpload(apiKey, payload, token)) {
         return false;
      }

      upload.set("token", token);
      upload.set("filename", "screenshot.png");
      upload.set("content_type", "image/png");
      uploadsArray.add(upload);     
   }

   // Upload the logfile
   payload = io::clip_middle_of_logfile(std::ifstream("stonehearth.log", std::ifstream::in), 256 * 1024);
   if (!PostRedmineUpload(apiKey, payload, token)) {
      return false;
   }
   upload.set("token", token);
   upload.set("filename", "stonehearth.log");
   upload.set("content_type", "text/plain");
   uploadsArray.add(upload);

   CLIENT_LOG(1) << "PostRedmineIssues: " << issues.write_formatted();
   issues.set("uploads", uploadsArray);
   payload = issues.write();

   std::string uri = BUILD_STRING("http://" << REDMINE_HOST << "/issues.json");
   Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_POST, uri, Poco::Net::HTTPMessage::HTTP_1_1);
   request.setContentType("application/json");
   request.setContentLength(payload.size());

   Poco::Net::HTTPClientSession session(REDMINE_HOST, REDMINE_PORT);
   std::ostream& request_stream = session.sendRequest(request);
   request_stream << payload;

   Poco::Net::HTTPResponse response;
   std::istream& response_stream = session.receiveResponse(response);

   std::istreambuf_iterator<char> eos;
   std::string s(std::istreambuf_iterator<char>(response_stream), eos);
   CLIENT_LOG(1) << "PostRedmineIssues response: " << s;
   boost::algorithm::replace_all(s, "\\\"", "\"");
   result = libjson::parse(s);

   int status = response.getStatus();
   return status == Poco::Net::HTTPResponse::HTTP_CREATED;
}

bool Client::PostRedmineUpload(std::string const& apiKey, std::string const& payload, std::string& uploadToken)
{
   std::string uri = BUILD_STRING("http://" << REDMINE_HOST << "/uploads.json");
   Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_POST, uri, Poco::Net::HTTPMessage::HTTP_1_1);
   request.set("X-Redmine-API-Key", apiKey.c_str());
   request.setContentType("application/octet-stream");
   request.setContentLength(payload.size());

   std::ostringstream os;
   request.write(os);
   CLIENT_LOG(1) << "PostRedmineUpload: " << os.str();

   Poco::Net::HTTPClientSession session(REDMINE_HOST, REDMINE_PORT);
   std::ostream& request_stream = session.sendRequest(request);
   request_stream << payload;

   Poco::Net::HTTPResponse response;
   std::istream& response_stream = session.receiveResponse(response);

   std::istreambuf_iterator<char> eos;
   std::string s(std::istreambuf_iterator<char>(response_stream), eos);
   CLIENT_LOG(1) << "PostRedmineUpload response: " << s;
   boost::algorithm::replace_all(s, "\\\"", "\"");
   json::Node result = libjson::parse(s);

   int status = response.getStatus();
   uploadToken = result.get("upload.token", "");
   return status == Poco::Net::HTTPResponse::HTTP_CREATED;
}