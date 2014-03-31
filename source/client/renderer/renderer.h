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
   int         frame_time;

   FrameStartInfo(int n, float i, int ft) : now(n), interpolate(i), frame_time(ft) { }
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
   RendererConfigEntry<int>  shadow_resolution;
   RendererConfigEntry<float> draw_distance;
   RendererConfigEntry<bool> use_fast_hilite;

   RendererConfigEntry<int> screen_width;
   RendererConfigEntry<int> screen_height;

   RendererConfigEntry<int> last_window_x;
   RendererConfigEntry<int> last_window_y;

   RendererConfigEntry<int> last_screen_x;
   RendererConfigEntry<int> last_screen_y;

   RendererConfigEntry<bool> enable_debug_keys;
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

   private:
      // Intermediate results from a raycast.
      struct _RayCastResult
      {
         csg::Point3f point;
         csg::Point3f normal;
         H3DNode node;
      };

      struct _RayCastResults {
         int num_results;
         csg::Ray3 ray;
         _RayCastResult results[10];
      };


   public:
      static Renderer& GetInstance();

      void SetRootEntity(om::EntityPtr rootObject);
      void Initialize();
      void Shutdown();

      void SetScriptHost(lua::ScriptHost* host);
      lua::ScriptHost* GetScriptHost() const;
      void DecodeDebugShapes(const ::radiant::protocol::shapelist& msg);
      void RenderOneFrame(int now, float alpha);
      void LoadResources();
      void ShowPerfHud(bool value);
      void SetServerTick(int tick);

      int GetWindowWidth() const;
      int GetWindowHeight() const;
      void SetUITextureSize(int width, int height);
      SystemStats GetStats();
      const RendererConfig& GetRendererConfig() const { return config_; }
      void ApplyConfig(const RendererConfig& newConfig, bool persistConfig);
      void PersistConfig();
      void UpdateConfig(const RendererConfig& newConfig);
      void SetupGlfwHandlers();
      void InitWindow();
      void InitHorde();
      void MakeRendererResources();

      void SelectSaneVideoMode(bool fullscreen, int* width, int* height, int* windowX, int* windowY, GLFWmonitor** monitor);
      void GetViewportMouseCoords(double& x, double& y);
      csg::Point2 GetMousePosition() const;
      csg::Matrix4 GetNodeTransform(H3DNode node) const;

      bool IsRunning() const;
      HWND GetWindowHandle() const;

      json::Node GetTerrainConfig() const;

      std::shared_ptr<RenderEntity> CreateRenderObject(H3DNode parent, om::EntityPtr obj);
      std::shared_ptr<RenderEntity> GetRenderObject(om::EntityPtr obj);
      std::shared_ptr<RenderEntity> GetRenderObject(int storeId, dm::ObjectId id);
      void RemoveRenderObject(int storeId, dm::ObjectId id);

      typedef std::function<void(om::Selection& sel, const csg::Ray3& ray, const csg::Point3f& intersection, const csg::Point3f& normal)> UpdateSelectionFn;
      core::Guard TraceSelected(H3DNode node, UpdateSelectionFn fn);

      void GetCameraToViewportRay(int viewportX, int viewportY, csg::Ray3* ray);
      void QuerySceneRay(int viewportX, int viewportY, int userFlags, RaycastResult &result);
      void QuerySceneRay(const csg::Point3f& origin, const csg::Point3f& direction, int userFlags, RaycastResult &result);

      typedef std::function<void (const Input&)> InputEventCb;
      void SetInputHandler(InputEventCb fn) { input_cb_ = fn; }

      core::Guard OnScreenResize(std::function<void(csg::Point2)> fn);
      core::Guard OnServerTick(std::function<void(int)> fn);
      core::Guard OnRenderFrameStart(std::function<void(FrameStartInfo const&)> fn);
      core::Guard OnShowDebugShapesChanged(std::function<void(bool)> fn);

      bool ShowDebugShapes();
      void SetShowDebugShapes(bool show_debug_shapes);

      void UpdateUITexture(const csg::Region2& rgn);

      Camera* GetCamera() { return camera_; }

      ViewMode GetViewMode() const { return viewMode_; }
      void SetViewMode(ViewMode mode);
      H3DRes GetPipeline(const std::string& name);
      bool ShouldHideRenderGrid(const csg::Point3& normal);

      void FlushMaterials();

      void SetSkyColors(const csg::Point3f& startCol, const csg::Point3f& endCol);
      void SetStarfieldBrightness(float brightness);

      void HandleResize();

      void* GetNextUiBuffer();
      void* GetLastUiBuffer();

      void SetDrawWorld(bool drawWorld);
      bool SetVisibleRegion(std::string const& visible_region_uri);
      bool SetExploredRegion(std::string const& explored_region_uri);

      std::string GetHordeResourcePath() const { return resourcePath_; }

   private:
      NO_COPY_CONSTRUCTOR(Renderer);
      RendererConfig config_;

   private:
      void RenderFogOfWarRT();
      H3DRes BuildSphereGeometry();
      void GetConfigOptions();
      void BuildSkySphere();
      void BuildStarfield();
      void SetStageEnable(H3DRes pipeRes, const char* stageName, bool enabled);
      void OnWindowResized(int newWidth, int newHeight);
      void OnKey(int key, int down);
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
      void UpdateFoW(H3DNode node, const csg::Region2& region);
      void CastRay(const csg::Point3f& origin, const csg::Point3f& direction, int userFlags, _RayCastResults* result);

      void ResizeViewport();
      void ResizePipelines();
      void CallWindowResizeListeners();

      void DispatchInputEvent();
      bool LoadMissingResources();
      void OneTimeIninitializtion();

   protected:
      struct RenderMapEntry {
         std::shared_ptr<RenderEntity>    render_entity;
         dm::TracePtr                     lifetime_trace;
      };
      typedef std::unordered_map<H3DNode, UpdateSelectionFn>   SelectableMap;
      typedef std::unordered_map<dm::ObjectId, RenderMapEntry> RenderEntityMap;
      typedef std::unordered_map<std::string, H3DRes>          H3DResourceMap;

      int               windowWidth_;
      int               windowHeight_;
      int               uiWidth_;
      int               uiHeight_;
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

      RenderEntityMap   entities_[5]; // by store id
      SelectableMap     selectableCbs_;
      InputEventCb      input_cb_;

      Input             input_;  // Mouse coordinates in the GL window-space.
      bool              initialized_;
      bool              iconified_;

      ViewMode          viewMode_;
      json::Node        terrainConfig_;
      lua::ScriptHost*  scriptHost_;

      core::BufferedSlot<csg::Point2>     screen_resize_slot_;
      core::BufferedSlot<bool>            show_debug_shapes_changed_slot_;
      core::Slot<int>                     server_tick_slot_;
      core::Slot<FrameStartInfo const&>   render_frame_start_slot_;
      std::unique_ptr<PerfHud>            perf_hud_;

      int               last_render_time_;
      bool              resize_pending_;
      bool              inFullscreen_;
      int               nextWidth_, nextHeight_;
      
      std::string       resourcePath_;
      std::string       lastGlfwError_;

      dm::TracePtr      visibilityTrace_;
      dm::TracePtr      exploredTrace_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_RENDERER_RENDERER_H
