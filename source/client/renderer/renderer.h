#ifndef _RADIANT_CLIENT_RENDERER_RENDERER_H
#define _RADIANT_CLIENT_RENDERER_RENDERER_H

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL

#include "namespace.h"
#include "Horde3D.h"
#include "csg/transform.h"
#include "csg/ray.h"
#include "csg/matrix.h"
#include "om/om.h"
#include "resources/namespace.h"
#include "protocols/tesseract.pb.h"
#include "csg/region.h"
#include "radiant_file.h"
#include <unordered_map>
#include <boost/property_tree/ptree.hpp>
#include "lib/perfmon/perfmon.h"
#include "lib/lua/lua.h"
#include "camera.h"
#include "platform/FileWatcher.h"
#include "core/input.h"
#include "core/buffered_slot.h"
#include "ui_buffer.h"
#include "glfw3.h"
#include <unordered_set>
#include "raycast_result.h"
#include "horde3d\Source\Horde3DEngine\egHudElement.h"

#define NUM_STORES 5

BEGIN_RADIANT_CLIENT_NAMESPACE

class RenderEntity;
class RenderDebugShape;

enum ViewMode {
   Standard = 0,
   RPG,
   MaxViewModes
};

struct FrameStartInfo {
   int         now;
   float       interpolate;
   int         frame_time;  // How long, in game-ms, was this frame?  (This is dependent on game-speed.)
   int         frame_time_wallclock;  // Wall-clock time.  (This is dependent on inertial reference frames.)

   FrameStartInfo(int n, float i, int ft, int ftw) : now(n), interpolate(i), frame_time(ft), frame_time_wallclock(ftw) { }
};

template<typename T>
struct RendererConfigEntry {
   T value;
   bool allowed;
};

struct RendererConfig {
   RendererConfigEntry<bool> enable_ssao;
   RendererConfigEntry<bool> use_shadows;
   RendererConfigEntry<bool> enable_vsync;
   RendererConfigEntry<bool> enable_fullscreen;
   RendererConfigEntry<bool> enable_gl_logging;
   RendererConfigEntry<int>  num_msaa_samples;
   RendererConfigEntry<int>  shadow_quality;
   RendererConfigEntry<int>  max_lights;
   RendererConfigEntry<float> draw_distance;
   RendererConfigEntry<bool> use_high_quality;

   RendererConfigEntry<int> screen_width;
   RendererConfigEntry<int> screen_height;

   RendererConfigEntry<int> last_window_x;
   RendererConfigEntry<int> last_window_y;

   RendererConfigEntry<int> last_screen_x;
   RendererConfigEntry<int> last_screen_y;

   RendererConfigEntry<bool> enable_debug_keys;

   RendererConfigEntry<bool> minimized;
   
   RendererConfigEntry<bool> disable_pinned_memory;
   RendererConfigEntry<bool> run_once;
};

struct SystemStats {
   float frame_rate;
   std::string cpu_info;
   std::string gpu_vendor;
   std::string gpu_renderer;
   std::string gl_version;
   int memory_gb;
};

class Renderer
{
   public:
      Renderer();
      ~Renderer();

   public:
      static Renderer& GetInstance();

      void SetRootEntity(om::EntityPtr rootObject);
      void Initialize();
      void Shutdown();

      void SetScriptHost(lua::ScriptHost* host);
      lua::ScriptHost* GetScriptHost() const;
      void DecodeDebugShapes(const ::radiant::protocol::shapelist& msg);
      void RenderOneFrame(int now, float alpha);
      void ConstructAllRenderEntities();
      void LoadResources();
      void ShowPerfHud(bool value);
      void SetServerTick(int tick);

      csg::Point2 const& GetScreenSize() const;
      csg::Point2 const& GetMinUiSize() const;

      SystemStats GetStats();
      const RendererConfig& GetRendererConfig() const { return config_; }

      enum ApplyConfigFlags {
         APPLY_CONFIG_RENDERER = (1 << 1),
         APPLY_CONFIG_PERSIST  = (1 << 2),
         APPLY_CONFIG_ALL      = -1,
      };
      void ApplyConfig(const RendererConfig& newConfig, int flags);
      void PersistConfig();
      void UpdateConfig(const RendererConfig& newConfig);
      void SetupGlfwHandlers();
      csg::Point2 InitWindow();
      void InitHorde();
      void MakeRendererResources();

      void SelectSaneVideoMode(bool fullscreen, csg::Point2 &pos, csg::Point2 &size, GLFWmonitor** monitor);
      void GetViewportMouseCoords(double& x, double& y);
      csg::Point2 GetMousePosition() const;
      csg::Matrix4 GetNodeTransform(H3DNode node) const;

      bool IsRunning() const;
      HWND GetWindowHandle() const;

      std::shared_ptr<RenderEntity> GetRenderEntity(om::EntityPtr obj);
      std::shared_ptr<RenderEntity> CreateRenderEntity(H3DNode parent, om::EntityPtr obj);
      
      typedef std::function<void(om::Selection& sel, const csg::Ray3& ray, const csg::Point3f& intersection, const csg::Point3f& normal)> UpdateSelectionFn;
      core::Guard SetSelectionForNode(H3DNode node, om::EntityRef e);

      void GetCameraToViewportRay(int viewportX, int viewportY, csg::Ray3* ray);
      RaycastResult QuerySceneRay(int viewportX, int viewportY, int userFlags);
      RaycastResult QuerySceneRay(const csg::Point3f& origin, const csg::Point3f& direction, int userFlags);

      typedef std::function<void (const Input&)> InputEventCb;
      void SetInputHandler(InputEventCb fn) { input_cb_ = fn; }

      core::Guard OnScreenResize(std::function<void(csg::Point2)> const& fn);
      core::Guard OnServerTick(std::function<void(int)> const& fn);
      core::Guard OnRenderFrameStart(std::function<void(FrameStartInfo const&)> const& fn);
      core::Guard OnRenderFrameFinished(std::function<void(FrameStartInfo const&)> const& fn);
      core::Guard OnShowDebugShapesChanged(std::function<void(bool)> const& fn);

      bool ShowDebugShapes();
      void SetShowDebugShapes(bool show_debug_shapes);

      void UpdateUITexture(csg::Point2 const& size, csg::Region2 const& rgn, const uint32* buff);

      Camera* GetCamera() { return camera_; }

      H3DRes GetPipeline(std::string const& name);

      void FlushMaterials();

      void SetSkyColors(const csg::Point3f& startCol, const csg::Point3f& endCol);
      void SetStarfieldBrightness(float brightness);

      void HandleResize();

      void SetDrawWorld(bool drawWorld);
      void SetLoading(bool loading);
      bool SetVisibleRegion(std::string const& visible_region_uri);
      bool SetExploredRegion(std::string const& explored_region_uri);

      std::string GetHordeResourcePath() const { return resourcePath_; }

      void UpdateLoadingProgress(float amountLoaded);

   private:
      NO_COPY_CONSTRUCTOR(Renderer);

      typedef std::function<void(csg::Point3f const& pt, csg::Point3f const& normal, H3DNode node)> RayCastHitCb;

   private:
      void RenderFogOfWarRT();
      void RenderLoadingMeter();
      H3DRes BuildSphereGeometry();
      void GetConfigOptions();
      void BuildSkySphere();
      void BuildStarfield();
      void BuildLoadingScreen();
      void SetStageEnable(H3DRes pipeRes, const char* stageName, bool enabled);
      void OnWindowResized(csg::Point2 const& size);
      void OnKey(int key, int down, int mods);
      void OnMouseWheel(double value);
      void OnMouseMove(double x, double y);
      void OnMouseButton(int button, int press);
      void OnMouseEnter(int entered);
      void OnFocusChanged(int wasFocused);
      void OnRawInput(UINT msg, WPARAM wParam, LPARAM lParam);
      void UpdateCamera();
      float DistFunc(float dist, int wheel, float minDist, float maxDist) const;
      MouseInput WindowToBrowser(const MouseInput& mouse);
      void CallMouseInputCallbacks();
      void UpdateFoW(H3DNode node, const csg::Region2f& region);
      void CastRay(const csg::Point3f& origin, const csg::Point3f& direction, int userFlags, RayCastHitCb const& cb);

      void ResizeViewport();
      void ResizePipelines();
      void CallWindowResizeListeners();

      void DispatchInputEvent();
      bool LoadMissingResources();
      void OneTimeIninitializtion();
      void SelectPipeline();
      void OnWindowClose();

      void MaskHighQualitySettings();

      std::string GetGraphicsCardName() const;
      void SelectRecommendedGfxLevel(std::string const& gfxCard);
      int GetGpuPerformance(std::string const& gfxCard) const;

   protected:
      struct RenderMapEntry {
         std::shared_ptr<RenderEntity>    render_entity;
         dm::TracePtr                     lifetime_trace;
      };
      RendererConfig config_;

      typedef std::unordered_map<H3DNode, om::EntityRef>       SelectionLookup;
      typedef std::unordered_map<dm::ObjectId, RenderMapEntry> RenderEntityMap;
      typedef std::unordered_map<std::string, H3DRes>          H3DResourceMap;

      csg::Point2       screenSize_;
      csg::Point2       uiSize_;
      csg::Point2       minUiSize_;
      bool              drawWorld_;
      uint32            fowRenderTarget_;

      H3DResourceMap    pipelines_;
   	std::string       currentPipeline_;

      H3DRes            fontMatRes_;
      H3DRes            panelMatRes_;

      UiBuffer          uiBuffer_;

      Camera            *camera_, *fowCamera_;
      FW::FileWatcher   fileWatcher_;

      std::string       worldPipeline_;

      H3DNode     fowExploredNode_, fowVisibleNode_;
      core::Guard           traces_;

      std::shared_ptr<RenderEntity> rootRenderObject_;
      H3DNode           debugShapes_;
      bool              show_debug_shapes_;

      RenderEntityMap   entities_[NUM_STORES]; // by store id
      SelectionLookup   selectionLookup_;
      InputEventCb      input_cb_;

      Input             input_;  // Mouse coordinates in the GL window-space.
      bool              initialized_;
      bool              iconified_;

      json::Node        terrainConfig_;
      lua::ScriptHost*  scriptHost_;

      core::BufferedSlot<csg::Point2>     screen_resize_slot_;
      core::BufferedSlot<bool>            show_debug_shapes_changed_slot_;
      core::Slot<int>                     server_tick_slot_;
      core::Slot<FrameStartInfo const&>   render_frame_start_slot_;
      core::Slot<FrameStartInfo const&>   render_frame_finished_slot_;
      std::unique_ptr<PerfHud>            perf_hud_;

      int               last_render_time_;
      int               last_render_time_wallclock_;
      bool              resize_pending_;
      bool              inFullscreen_;
      csg::Point2       nextScreenSize_;
      int               _maxRenderEntityLoadTime;
      
      std::string       resourcePath_;
      std::string       lastGlfwError_;

      dm::TracePtr      visibilityTrace_;
      dm::TracePtr      exploredTrace_;
      std::vector<std::weak_ptr<RenderEntity>>  _newRenderEntities;

      bool              _loading;
      float             _loadingAmount;
      H3DRes            _loadingBackgroundMaterial;
      H3DRes            _loadingProgressMaterial;
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_RENDERER_RENDERER_H
