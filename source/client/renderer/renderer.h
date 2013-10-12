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
#include "lua/namespace.h"
#include "camera.h"
#include "platform/FileWatcher.h"
#include "core/input.h"
#include "core/buffered_slot.h"

IN_RADIANT_LUA_NAMESPACE(
   class ScriptHost;
)

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


class Renderer
{
   public:
      Renderer();
      ~Renderer();

   public:
      static Renderer& GetInstance();

      void Initialize(om::EntityPtr rootObject);
      void SetServerTick(int tick);
      void SetScriptHost(lua::ScriptHost* host);
      lua::ScriptHost* GetScriptHost() const;
      void DecodeDebugShapes(const ::radiant::protocol::shapelist& msg);
      void RenderOneFrame(int now, float alpha);
      void Cleanup();
      void LoadResources();

      int GetWidth() const;
      int GetHeight() const;
      void SetUITextureSize(int width, int height);

      csg::Point2 GetMousePosition() const;
      csg::Matrix4 GetNodeTransform(H3DNode node) const;

      bool IsRunning() const;
      HWND GetWindowHandle() const;

      boost::property_tree::ptree const& GetConfig() const;

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

      void UpdateUITexture(const csg::Region2& rgn, const char* buffer);

      Camera* GetCamera() { return camera_; }

      ViewMode GetViewMode() const { return viewMode_; }
      void SetViewMode(ViewMode mode);
      void SetCurrentPipeline(std::string pipeline);
      bool ShouldHideRenderGrid(const csg::Point3& normal);

      core::Guard TraceFrameStart(std::function<void()> fn);
      core::Guard TraceInterpolationStart(std::function<void()> fn);

      int GetCurrentFrameTime() const { return currentFrameTime_; }
      float GetCurrentFrameInterp() const { return currentFrameInterp_; }
      float GetLastFrameRenderTime() const { return lastFrameTimeInSeconds_; }

      void FlushMaterials();

   private:
      NO_COPY_CONSTRUCTOR(Renderer);

   private:
      void OnWindowResized(int newWidth, int newHeight);
      void OnKey(int key, int down);
      void OnMouseWheel(double value);
      void OnMouseMove(double x, double y);
      void OnMouseButton(int button, int press);
      void OnMouseEnter(int entered);
      void OnRawInput(UINT msg, WPARAM wParam, LPARAM lParam);
      void Resize(int width, int height);
      void UpdateCamera();
      float DistFunc(float dist, int wheel, float minDist, float maxDist) const;
      MouseInput WindowToBrowser(const MouseInput& mouse);
      void CallMouseInputCallbacks();

      typedef std::unordered_map<dm::TraceId, std::function<void()>> TraceMap;

      core::Guard AddTrace(TraceMap& m, std::function<void()> fn);
      void RemoveTrace(TraceMap& m, dm::TraceId tid);
      void FireTraces(const TraceMap& m);
      void DispatchInputEvent();

   protected:
      typedef std::unordered_map<H3DNode, UpdateSelectionFn> SelectableMap;
      typedef std::unordered_map<dm::ObjectId, std::shared_ptr<RenderEntity>> RenderEntityMap;
      typedef std::unordered_map<std::string, H3DRes>    H3DResourceMap;
      int               width_;
      int               height_;
      int               uiWidth_;
      int               uiHeight_;

      H3DResourceMap    pipelines_;
   	H3DRes            currentPipeline_;

      H3DRes            logoMatRes_;
      H3DRes            fontMatRes_;
      H3DRes            panelMatRes_;
      H3DRes            uiMatRes_;
      H3DRes            uiTexture_;

      Camera*            camera_;
      FW::FileWatcher   fileWatcher_;

      dm::TraceId       nextTraceId_;
      core::Guard           traces_;
      TraceMap          renderFrameTraces_;
      TraceMap          interpolationStartTraces_;

      std::shared_ptr<RenderEntity>      rootRenderObject_;
      H3DNode                       debugShapes_;

      RenderEntityMap               entities_[5]; // by store id
      SelectableMap                 selectableCbs_;
      InputEventCb                  input_cb_;

      Input                         input_;  // Mouse coordinates in the GL window-space.
      bool                          initialized_;

      int                           currentFrameTime_;
      float                         currentFrameInterp_;
      float                         lastFrameTimeInSeconds_;
      ViewMode                      viewMode_;
      boost::property_tree::basic_ptree<std::string, std::string> config_;
      lua::ScriptHost*              scriptHost_;

      core::BufferedSlot<csg::Point2>  screen_resize_slot_;
      std::unique_ptr<PerfHud>         perf_hud_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_RENDERER_RENDERER_H
