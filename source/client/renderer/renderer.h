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
#include "tesseract.pb.h"
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

BEGIN_RADIANT_CLIENT_NAMESPACE

class RenderEntity;
class RenderDebugShape;

enum ViewMode {
   Standard = 0,
   RPG,
   MaxViewModes
};

struct RayCastResult
{
   csg::Point3f point;
   csg::Point3f normal;
   
   csg::Point3f origin;
   csg::Point3f direction;

   H3DNode      node;
   bool         is_valid;
};

struct FrameStartInfo {
   int         now;
   float       interpolate;

   FrameStartInfo(int n, float i) : now(n), interpolate(i) { }
};

struct RendererConfig {
   bool use_forward_renderer;
   bool use_ssao;
   bool use_ssao_blur;
   bool use_shadows;
   int  num_msaa_samples;
   int  shadow_resolution;
};

struct SystemStats {
   float frameRate;
   std::string cpuInfo;
   std::string gpuInfo;
   int memInfo;
};

class Renderer
{
   public:
      Renderer();
      ~Renderer();

   public:
      static Renderer& GetInstance();
      static void GetConfigOptions();
      static RendererConfig config_;

      void Initialize(om::EntityPtr rootObject);
      void SetScriptHost(lua::ScriptHost* host);
      lua::ScriptHost* GetScriptHost() const;
      void DecodeDebugShapes(const ::radiant::protocol::shapelist& msg);
      void RenderOneFrame(int now, float alpha);
      void Cleanup();
      void LoadResources();
      void ShowPerfHud(bool value);
      void SetServerTick(int tick);
      void ApplyConfig();

      int GetWidth() const;
      int GetHeight() const;
      void SetUITextureSize(int width, int height);
	  SystemStats GetStats();

      csg::Point2 GetMousePosition() const;
      csg::Matrix4 GetNodeTransform(H3DNode node) const;

      bool IsRunning() const;
      HWND GetWindowHandle() const;

      boost::property_tree::ptree const& GetTerrainConfig() const;

      std::shared_ptr<RenderEntity> CreateRenderObject(H3DNode parent, om::EntityPtr obj);
      std::shared_ptr<RenderEntity> GetRenderObject(om::EntityPtr obj);
      std::shared_ptr<RenderEntity> GetRenderObject(int storeId, dm::ObjectId id);
      void RemoveRenderObject(int storeId, dm::ObjectId id);

      typedef std::function<void(om::Selection& sel, const csg::Ray3& ray, const csg::Point3f& intersection, const csg::Point3f& normal)> UpdateSelectionFn;
      core::Guard TraceSelected(H3DNode node, UpdateSelectionFn fn);

      void GetCameraToViewportRay(int windowX, int windowY, csg::Ray3* ray);
      void CastRay(const csg::Point3f& origin, const csg::Point3f& direction, RayCastResult* result);
      void CastScreenCameraRay(int windowX, int windowY, RayCastResult* result);
      void QuerySceneRay(int windowX, int windowY, om::Selection &result);

      typedef std::function<void (const Input&)> InputEventCb;
      void SetInputHandler(InputEventCb fn) { input_cb_ = fn; }

      core::Guard OnScreenResize(std::function<void(csg::Point2)> fn);
      core::Guard OnServerTick(std::function<void(int)> fn);
      core::Guard OnRenderFrameStart(std::function<void(FrameStartInfo const&)> fn);
      core::Guard OnShowDebugShapesChanged(std::function<void(bool)> fn);

      bool GetShowDebugShapes();
      void SetShowDebugShapes(bool show_debug_shapes);

      void UpdateUITexture(const csg::Region2& rgn, const char* buffer);

      Camera* GetCamera() { return camera_; }

      ViewMode GetViewMode() const { return viewMode_; }
      void SetViewMode(ViewMode mode);
      void SetCurrentPipeline(std::string pipeline);
      bool ShouldHideRenderGrid(const csg::Point3& normal);

      void FlushMaterials();

   private:
      NO_COPY_CONSTRUCTOR(Renderer);

   private:
      void SetStageEnable(const char* stageName, bool enabled);
      void OnWindowResized(int newWidth, int newHeight);
      void OnKey(int key, int down);
      void OnMouseWheel(double value);
      void OnMouseMove(double x, double y);
      void OnMouseButton(int button, int press);
      void OnMouseEnter(int entered);
      void OnRawInput(UINT msg, WPARAM wParam, LPARAM lParam);
      void UpdateCamera();
      float DistFunc(float dist, int wheel, float minDist, float maxDist) const;
      MouseInput WindowToBrowser(const MouseInput& mouse);
      void CallMouseInputCallbacks();

      void ResizeWindow(int width, int height);
      void ResizeViewport();
      void ResizePipelines();

      void DispatchInputEvent();

   protected:
      typedef std::unordered_map<H3DNode, UpdateSelectionFn> SelectableMap;
      typedef std::unordered_map<dm::ObjectId, std::shared_ptr<RenderEntity>> RenderEntityMap;
      typedef std::unordered_map<std::string, H3DRes>    H3DResourceMap;
      int               windowWidth_;
      int               windowHeight_;
      int               uiWidth_;
      int               uiHeight_;

      H3DResourceMap    pipelines_;
   	H3DRes            currentPipeline_;

      H3DRes            fontMatRes_;
      H3DRes            panelMatRes_;
      H3DRes            uiMatRes_;
      H3DRes            uiTexture_;

      Camera*            camera_;
      FW::FileWatcher   fileWatcher_;

      core::Guard           traces_;

      std::shared_ptr<RenderEntity>      rootRenderObject_;
      H3DNode                       debugShapes_;
      bool                          show_debug_shapes_;

      RenderEntityMap               entities_[5]; // by store id
      SelectableMap                 selectableCbs_;
      InputEventCb                  input_cb_;

      Input                         input_;  // Mouse coordinates in the GL window-space.
      bool                          initialized_;

      ViewMode                      viewMode_;
      boost::property_tree::basic_ptree<std::string, std::string> terrainConfig_;
      lua::ScriptHost*              scriptHost_;

      core::BufferedSlot<csg::Point2>     screen_resize_slot_;
      core::BufferedSlot<bool>            show_debug_shapes_changed_slot_;
      core::Slot<int>                     server_tick_slot_;
      core::Slot<FrameStartInfo const&>   render_frame_start_slot_;
      std::unique_ptr<PerfHud>            perf_hud_;
      H3DRes                              uiPbo_;
      int                                 last_render_time_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_RENDERER_RENDERER_H
