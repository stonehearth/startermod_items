#ifndef _RADIANT_CLIENT_RENDERER_RENDERER_H
#define _RADIANT_CLIENT_RENDERER_RENDERER_H

#include "namespace.h"
#include "Horde3D.h"
#include "math3d.h"
#include "om/om.h"
#include "resources/namespace.h"
#include "tesseract.pb.h"
#include "csg/region.h"
#include "radiant_file.h"
#include <unordered_map>
#include <boost/property_tree/ptree.hpp>
#include "lua/namespace.h"

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

      int GetUIWidth() const;
      int GetUIHeight() const;
      int GetWidth() const;
      int GetHeight() const;

      csg::Point2 GetMousePosition() const;
      math3d::matrix4 GetNodeTransform(H3DNode node) const;

      bool IsRunning() const;
      HWND GetWindowHandle() const;

      boost::property_tree::ptree const& GetConfig() const;

      std::shared_ptr<RenderEntity> CreateRenderObject(H3DNode parent, om::EntityPtr obj);
      std::shared_ptr<RenderEntity> GetRenderObject(om::EntityPtr obj);
      std::shared_ptr<RenderEntity> GetRenderObject(int storeId, om::EntityId id);
      void RemoveRenderObject(int storeId, om::EntityId id);

      typedef std::function<void(om::Selection& sel, const math3d::ray3& ray, const math3d::point3& intersection, const math3d::point3& normal)> UpdateSelectionFn;
      dm::Guard TraceSelected(H3DNode node, UpdateSelectionFn fn);

      math3d::ray3 GetCameraToViewportRay(int windowX, int windowY);
      void QuerySceneRay(int windowX, int windowY, om::Selection &result);

      typedef int InputCallbackId;
      typedef std::function<void (const RawInputEvent&, bool& handled, bool& uninstall)> RawInputEventCb;
      typedef std::function<void (const MouseEvent&, bool& handled, bool& uninstall)> MouseEventCb;
      typedef std::function<void (const KeyboardEvent&, bool& handled, bool& uninstall)> KeyboardInputEventCb;
      InputCallbackId SetRawInputCallback(RawInputEventCb fn);
      InputCallbackId SetMouseInputCallback(MouseEventCb fn);
      InputCallbackId SetKeyboardInputCallback(KeyboardInputEventCb fn);
      void RemoveInputEventHandler(InputCallbackId id);

      void PointCamera(const math3d::point3 &location);
      void UpdateUITexture(const csg::Region2& rgn, const char* buffer);

      ViewMode GetViewMode() const { return viewMode_; }
      void SetViewMode(ViewMode mode);
      void SetCurrentPipeline(std::string pipeline);
      bool ShouldHideRenderGrid(const math2d::Point2d& normal);

      dm::Guard TraceFrameStart(std::function<void()> fn);
      dm::Guard TraceInterpolationStart(std::function<void()> fn);

      int GetCurrentFrameTime() const { return currentFrameTime_; }
      float GetCurrentFrameInterp() const { return currentFrameInterp_; }

   private:
      void OnKey(int key, int down);
      void OnMouseWheel(int value);
      void OnMouseMove(int x, int y);
      void OnMouseButton(int button, int press);
      void OnRawInput(UINT msg, WPARAM wParam, LPARAM lParam);
      void Resize(int width, int height);
      void UpdateCamera();
      math3d::quaternion GetCameraRotation();
      void CallMouseInputCallbacks();

      typedef std::unordered_map<dm::TraceId, std::function<void()>> TraceMap;

      dm::Guard AddTrace(TraceMap& m, std::function<void()> fn);
      void RemoveTrace(TraceMap& m, dm::TraceId tid);
      void FireTraces(const TraceMap& m);

   protected:
      typedef std::unordered_map<H3DNode, UpdateSelectionFn> SelectableMap;
      typedef std::unordered_map<om::EntityId, std::shared_ptr<RenderEntity>> RenderEntityMap;
      typedef std::vector<std::pair<InputCallbackId, RawInputEventCb>> RawInputCallbackMap;
      typedef std::vector<std::pair<InputCallbackId, MouseEventCb>> MouseInputCallbackMap;
      typedef std::vector<std::pair<InputCallbackId, KeyboardInputEventCb>> KeyboardInputCallbackMap;
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
      H3DNode           camera_;

      math3d::vector3   cameraPos_;
      math3d::vector3   cameraTarget_;
      math3d::vector3   cameraMoveDirection_;

      dm::TraceId       nextTraceId_;
      dm::GuardSet        traces_;
      TraceMap          renderFrameTraces_;
      TraceMap          interpolationStartTraces_;

      std::shared_ptr<RenderEntity>      rootRenderObject_;
      H3DNode                       debugShapes_;

      RenderEntityMap               entities_[5]; // by store id
      SelectableMap                 selectableCbs_;
      InputCallbackId               nextInputCbId_;
      RawInputCallbackMap           rawInputCbs_;
      MouseInputCallbackMap         mouseInputCbs_;
      KeyboardInputCallbackMap      keyboardInputCbs_;

      bool                          rotateCamera_;
      MouseEvent                    mouse_;
      bool                          initialized_;

      int                           currentFrameTime_;
      float                         currentFrameInterp_;
      ViewMode                      viewMode_;
      boost::property_tree::basic_ptree<std::string, std::string> config_;
      lua::ScriptHost*              scriptHost_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_RENDERER_RENDERER_H
