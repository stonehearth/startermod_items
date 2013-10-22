#include "pch.h"
#include "radiant.h"
#include "radiant_stdutil.h"
#include "core/config.h"
#include "renderer.h"
#include "Horde3DUtils.h"
#include "Horde3DRadiant.h"
#include "glfw3.h"
#include "glfw3native.h"
#include "render_entity.h"
#include "om/selection.h"
#include "om/entity.h"
#include <boost/property_tree/json_parser.hpp>
#include <SFML/Audio.hpp>
#include "camera.h"
#include "lib/perfmon/perfmon.h"
#include "perfhud/perfhud.h"

using namespace ::radiant;
using namespace ::radiant::client;
std::vector<float> ssaoSamplerData;
H3DRes ssaoMat;

static std::unique_ptr<Renderer> renderer_;
Renderer& Renderer::GetInstance()
{
   if (!renderer_) {
      assert(renderer_.get() == nullptr);
      new Renderer(); // will see the singleton in the constructor
      assert(renderer_.get() != nullptr);
   }
   return *renderer_;
}

Renderer::Renderer() :
   initialized_(false),
   viewMode_(Standard),
   scriptHost_(nullptr),
   nextTraceId_(1),
   camera_(nullptr),
   currentFrameTime_(0),
   lastFrameTimeInSeconds_(0),
   uiWidth_(0),
   uiHeight_(0),
   uiTexture_(0),
   uiMatRes_(0),
   uiPbo_(0)

{
   try {

      boost::property_tree::json_parser::read_json("config/renderer.json", config_);
   } catch(boost::property_tree::json_parser::json_parser_error &e) {
      LOG(WARNING) << "Error parsing: " << e.filename() << " on line: " << e.line() << std::endl;
      LOG(WARNING) << e.message() << std::endl;
   }

   assert(renderer_.get() == nullptr);
   renderer_.reset(this);

   width_ = 1920;
   height_ = 1080;

   glfwInit();

   GLFWwindow *window;
   // Fullscreen: add glfwGetPrimaryMonitor() instead of the first NULL.
   if (!(window = glfwCreateWindow(width_, height_, "Stonehearth", NULL, NULL))) {
      glfwTerminate();
   }

   glfwMakeContextCurrent(window);
   
	glfwSwapInterval(0);

	if (!h3dInit()) {	
		h3dutDumpMessages();
      return;
   }

	// Set options
	h3dSetOption(H3DOptions::LoadTextures, 1);
	h3dSetOption(H3DOptions::TexCompression, 0);
	h3dSetOption(H3DOptions::MaxAnisotropy, 4);
	h3dSetOption(H3DOptions::ShadowMapSize, 2048);
	h3dSetOption(H3DOptions::FastAnimation, 1);
   h3dSetOption(H3DOptions::DumpFailedShaders, 1);
   h3dSetOption(H3DOptions::SampleCount, 4);

   SetCurrentPipeline("pipelines/forward.pipeline.xml");

   // Overlays
	fontMatRes_ = h3dAddResource( H3DResTypes::Material, "overlays/font.material.xml", 0 );
	panelMatRes_ = h3dAddResource( H3DResTypes::Material, "overlays/panel.material.xml", 0 );


   H3DRes skyBoxRes = h3dAddResource( H3DResTypes::SceneGraph, "models/skybox/skybox.scene.xml", 0 );
   
   // xxx - should move this into the horde extension for debug shapes, but it doesn't know
   // how to actually get the resource loaded!
   h3dAddResource(H3DResTypes::Material, "materials/debug_shape.material.xml", 0); 
   
   ssaoMat = h3dAddResource(H3DResTypes::Material, "materials/ssao.material.xml", 0);

   H3DRes veclookup = h3dCreateTexture("RandomVectorLookup", 4, 4, H3DFormats::TEX_RGBA32F, H3DResFlags::NoTexMipmaps);
   float *data2 = (float *)h3dMapResStream(veclookup, H3DTexRes::ImageElem, 0, H3DTexRes::ImgPixelStream, false, true);
   for (int i = 0; i < 16; i++)
   {
      float x = ((rand() / (float)RAND_MAX) * 2.0f) - 1.0f;
      float y = ((rand() / (float)RAND_MAX) * 2.0f) - 1.0f;
      //float z = ((rand() / (float)RAND_MAX) * 2.0f) - 1.0f;
      float z = 0;
      Horde3D::Vec3f v(x,y,z);
      v.normalize();

      data2[(i * 4) + 0] = v.x;
      data2[(i * 4) + 1] = v.y;
      data2[(i * 4) + 2] = v.z;
   }
   h3dUnmapResStream(veclookup);
   LoadResources();

   // Sampler kernel generation--a work in progress.
   const int KernelSize = 16;
	for (int i = 0; i < KernelSize; ++i) {
      float x = ((rand() / (float)RAND_MAX) * 2.0f) - 1.0f;
      float y = ((rand() / (float)RAND_MAX) * 2.0f) - 1.0f;
      float z = ((rand() / (float)RAND_MAX) * -1.0f);
      Horde3D::Vec3f v(x,y,z);
      v.normalize();

		float scale = (float)i / (float)KernelSize;
      float f = scale;// * scale;
		v *= ((1.0f - f) * 0.3f) + (f);

      ssaoSamplerData.push_back(v.x);
      ssaoSamplerData.push_back(v.y);
      ssaoSamplerData.push_back(v.z);
      ssaoSamplerData.push_back(0.0);
	}

	// Add camera   
   camera_ = new Camera(H3DRootNode, "Camera", currentPipeline_);

   h3dSetNodeParamI(camera_->GetNode(), H3DCamera::PipeResI, currentPipeline_);
   // Resize
   Resize(width_, height_);

   memset(&input_.mouse, 0, sizeof input_.mouse);

   glfwSetWindowSizeCallback(window, [](GLFWwindow *window, int newWidth, int newHeight) { 
      Renderer::GetInstance().OnWindowResized(newWidth, newHeight); 
   });

   glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scancode, int action, int mods) { 
      Renderer::GetInstance().OnKey(key, (action == GLFW_PRESS) || (action == GLFW_REPEAT)); 
   });

   glfwSetCursorEnterCallback(window, [](GLFWwindow *window, int entered) {
      Renderer::GetInstance().OnMouseEnter(entered);
   });

   glfwSetCursorPosCallback(window, [](GLFWwindow *window, double x, double y) {      
      Renderer::GetInstance().OnMouseMove(x, y);
   });

   glfwSetMouseButtonCallback(window, [](GLFWwindow *window, int button, int action, int mods) { 
      Renderer::GetInstance().OnMouseButton(button, action == GLFW_PRESS); 
   });

   glfwSetScrollCallback(window, [](GLFWwindow *window, double xoffset, double yoffset) {
      Renderer::GetInstance().OnMouseWheel(yoffset);
   });

   // Platform-specific ifdef code goes here....
   glfwSetRawInputCallbackWin32(window, [](GLFWwindow *window, UINT uMsg, WPARAM wParam, LPARAM lParam) {
      Renderer::GetInstance().OnRawInput(uMsg, wParam, lParam);
   });
   
   glfwSetWindowCloseCallback(window, [](GLFWwindow* window) -> void {
      // die RIGHT NOW!!
      LOG(WARNING) << "Bailing...";
      TerminateProcess(GetCurrentProcess(), 1);
   });
   SetWindowPos(GetWindowHandle(), NULL, 0, 0 , 0, 0, SWP_NOSIZE);

   fileWatcher_.addWatch(strutil::utf8_to_unicode("horde"), [](FW::WatchID watchid, const std::wstring& dir, const std::wstring& filename, FW::Action action) -> void {
      Renderer::GetInstance().FlushMaterials();
   }, true);

   initialized_ = true;
}

void Renderer::ShowPerfHud(bool value) {
   if (value && !perf_hud_) {
      perf_hud_.reset(new PerfHud(*this));
   } else if (!value && perf_hud_) {
      delete perf_hud_.release();
   }
}

void Renderer::FlushMaterials() {
   H3DRes r = 0;
   while ((r = h3dGetNextResource(H3DResTypes::Shader, r)) != 0) {
      h3dUnloadResource(r);
   }

   r = 0;
   while ((r = h3dGetNextResource(H3DResTypes::Material, r)) != 0) {
      // TODO: create a set of manually-loaded materials.
      if (r != uiMatRes_) {
         h3dUnloadResource(r);
      }
   }

   r = 0;
   while ((r = h3dGetNextResource(H3DResTypes::Pipeline, r)) != 0) {
      h3dUnloadResource(r);
   }

   r = 0;
   while ((r = h3dGetNextResource(H3DResTypes::ParticleEffect, r)) != 0) {
      h3dUnloadResource(r);
   }

   r = 0;
   while ((r = h3dGetNextResource(H3DResTypes::Code, r)) != 0) {
      h3dUnloadResource(r);
   }

   r = 0;
   while ((r = h3dGetNextResource(RT_CubemitterResource, r)) != 0) {
      h3dUnloadResource(r);
   }

   r = 0;
   while ((r = h3dGetNextResource(RT_AnimatedLightResource, r)) != 0) {
      h3dUnloadResource(r);
   }

   Resize(width_, height_);
}

Renderer::~Renderer()
{
   glfwDestroyWindow(glfwGetCurrentContext());
   glfwTerminate();
}

void Renderer::Initialize(om::EntityPtr rootObject)
{
   rootRenderObject_ = CreateRenderObject(H3DRootNode, rootObject);
   debugShapes_ = h3dRadiantAddDebugShapes(H3DRootNode, "renderer debug shapes");
}

void Renderer::Cleanup()
{
   rootRenderObject_ = NULL;
   for (auto& e : entities_) {
      e.clear();
   }
   
   ASSERT(RenderEntity::GetTotalObjectCount() == 0);
}

void Renderer::SetServerTick(int tick)
{
   FireTraces(interpolationStartTraces_);
}

void Renderer::DecodeDebugShapes(const ::radiant::protocol::shapelist& msg)
{
   h3dRadiantDecodeDebugShapes(debugShapes_, msg);
}

HWND Renderer::GetWindowHandle() const
{
   return glfwGetWin32Window(glfwGetCurrentContext());
}

void Renderer::RenderOneFrame(int now, float alpha)
{
   perfmon::TimelineCounterGuard tcg("render one");

   int deltaNow = now - currentFrameTime_;

   lastFrameTimeInSeconds_ = deltaNow / 1000.0f;
   currentFrameTime_ =  now;
   currentFrameInterp_ = alpha;

   bool debug = glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_SPACE) == GLFW_PRESS;
   bool showStats = glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
  
   bool showUI = true;
   const float ww = (float)h3dGetNodeParamI(camera_->GetNode(), H3DCamera::ViewportWidthI) /
                    (float)h3dGetNodeParamI(camera_->GetNode(), H3DCamera::ViewportHeightI);

   h3dSetOption(H3DOptions::DebugViewMode, debug);
   h3dSetOption(H3DOptions::WireframeMode, debug);
   // h3dSetOption(H3DOptions::DebugViewMode, _debugViewMode ? 1.0f : 0.0f);
   // h3dSetOption(H3DOptions::WireframeMode, _wireframeMode ? 1.0f : 0.0f);
	
   h3dSetCurrentRenderTime(now / 1000.0f);

   if (showUI && uiMatRes_) { // show UI
      const float ovUI[] = { 0,  0, 0, 1, // flipped up-side-down!
                             0,  1, 0, 0,
                             ww, 1, 1, 0,
                             ww, 0, 1, 1, };
      h3dShowOverlays(ovUI, 4, 1, 1, 1, 1, uiMatRes_, 0);
   }
   if (false) { // show color mat
      const float v[] = { ww * .9f, .9f,    0, 1, // flipped up-side-down!
                          ww * .9f,  1,     0, 0,
                          ww,        1,     1, 0,
                          ww,       .9f,    1, 1, };
   }

   perfmon::SwitchToCounter("render fire traces") ;
   FireTraces(renderFrameTraces_);


   if (showStats) { 
      // show stats
      h3dutShowFrameStats( fontMatRes_, panelMatRes_, H3DUTMaxStatMode );
   }

   perfmon::SwitchToCounter("render load res");
   fileWatcher_.update();
   LoadResources();

   h3dSetMaterialArrayUniform( ssaoMat, "samplerKernel", ssaoSamplerData.data(), ssaoSamplerData.size());

   // Render scene
   perfmon::SwitchToCounter("render h3d");
   h3dRender(camera_->GetNode());

   // Finish rendering of frame
   perfmon::SwitchToCounter("render finalize");
   h3dFinalizeFrame();
   //glFinish();

   // Advance emitter time; this must come AFTER rendering, because we only know which emitters
   // to update after doing a render pass.
   perfmon::SwitchToCounter("render ce");
   h3dRadiantAdvanceCubemitterTime(deltaNow / 1000.0f);
   h3dRadiantAdvanceAnimatedLightTime(deltaNow / 1000.0f);

   // Remove all overlays
   h3dClearOverlays();

   // Write all messages to log file
   h3dutDumpMessages();

   perfmon::SwitchToCounter("render swap");
   glfwSwapBuffers(glfwGetCurrentContext());
}

bool Renderer::IsRunning() const
{
   return true;
}

void Renderer::GetCameraToViewportRay(int windowX, int windowY, csg::Ray3* ray)
{
   // compute normalized window coordinates in preparation for casting a ray
   // through the scene
   float nwx = ((float)windowX) / width_;
   float nwy = 1.0f - ((float)windowY) / height_;

   // calculate the ray starting at the eye position of the camera, casting
   // through the specified window coordinates into the scene
   h3dutPickRay(camera_->GetNode(), nwx, nwy,
                &ray->origin.x, &ray->origin.y, &ray->origin.z,
                &ray->direction.x, &ray->direction.y, &ray->direction.z);
}

void Renderer::CastRay(const csg::Point3f& origin, const csg::Point3f& direction, RayCastResult* result)
{
   if (!rootRenderObject_) {
      return;
   }

   result->is_valid = false;
   if (h3dCastRay(rootRenderObject_->GetNode(),
      origin.x, origin.y, origin.z,
      direction.x, direction.y, direction.z, 1) == 0) {
		return;
	}

   // Pull out the intersection node and intersection point
	H3DNode node = 0;
	if (!h3dGetCastRayResult( 0, &(result->node), 0, &(result->point.x), &(result->normal.x) )) {
      return;
   }
   result->origin = origin;
   result->direction = direction;
   result->normal.Normalize();
   result->is_valid = true;
}

void Renderer::CastScreenCameraRay(int windowX, int windowY, RayCastResult* result)
{
   result->is_valid = false;

   if (!rootRenderObject_) {
      return;
   }

   csg::Ray3 ray;
   GetCameraToViewportRay(windowX, windowY, &ray);

   CastRay(ray.origin, ray.direction, result);
}

void Renderer::QuerySceneRay(int windowX, int windowY, om::Selection &result)
{
   RayCastResult r;
   CastScreenCameraRay(windowX, windowY, &r);
   if (!r.is_valid) {
      return;
   }

   result.AddLocation(csg::Point3f(r.point.x, r.point.y, r.point.z),
                      csg::Point3f(r.normal.x, r.normal.y, r.normal.z));

   // Lookup the intersection object in the node registery and ask it to
   // fill in the selection structure.
   H3DNode node = r.node;
   csg::Ray3 ray(r.point, r.direction);
   while (node) {
      const char *name = h3dGetNodeParamStr(node, H3DNodeParams::NameStr);
      auto i = selectableCbs_.find(node);
      if (i != selectableCbs_.end()) {
         // transform the interesection and the normal to the coordinate space
         // of the node.
         csg::Matrix4 transform = GetNodeTransform(node);
         transform.affine_inverse();

         i->second(result, ray, transform.transform(r.point), transform.rotate(r.normal));
      }
      node = h3dGetNodeParent(node);
   }
}

csg::Matrix4 Renderer::GetNodeTransform(H3DNode node) const
{
   const float *abs;
   h3dGetNodeTransMats(node, NULL, &abs);

   csg::Matrix4 transform;
   transform.fill(abs);

   return transform;
 }

void Renderer::UpdateUITexture(const csg::Region2& rgn, const char* buffer)
{
   if (!rgn.IsEmpty() && uiPbo_) {
      auto bounds = rgn.GetBounds();
      ASSERT(bounds.GetMax().x <= uiWidth_);
      ASSERT(bounds.GetMax().y <= uiHeight_);

      perfmon::SwitchToCounter("map ui pbo");

      char *data = (char *)h3dMapResStream(uiPbo_, 0, 0, 0, false, true);

      if (data) {
         perfmon::SwitchToCounter("copy client mem to ui pbo");

         // We can't loop through the individual rects and copy only them, because the PBO won't have
         // all the pixels in between (and we do NOT want to flush the renderer just to get them), so
         // this is a good compromise.

         int srcPitch = uiWidth_ * 4;
         int destPitch = bounds.GetWidth() * 4;
         int xStart = bounds.GetMin().x * 4;
         int yStart = bounds.GetMin().y;
         int amount = bounds.GetWidth() * 4;

         for (int y = yStart; y < bounds.GetMax().y; y++) {
            char* dst = data + ((y - yStart) * destPitch);
            const char* src = buffer + (y * srcPitch) + xStart;
            memcpy(dst, src, amount);
         }
      }
      perfmon::SwitchToCounter("unmap ui pbo");
      h3dUnmapResStream(uiPbo_);

      perfmon::SwitchToCounter("copy ui pbo to ui texture") ;

      h3dCopyBufferToBuffer(uiPbo_, uiTexture_, bounds.GetMin().x, bounds.GetMin().y, 
         bounds.GetWidth(), bounds.GetHeight());
   }
}

void Renderer::Resize( int width, int height )
{
   width_ = width;
   height_ = height;

   SetUITextureSize(width, height);
   
   H3DNode camera = camera_->GetNode();

   // Resize viewport
   h3dSetNodeParamI( camera, H3DCamera::ViewportXI, 0 );
   h3dSetNodeParamI( camera, H3DCamera::ViewportYI, 0 );
   h3dSetNodeParamI( camera, H3DCamera::ViewportWidthI, width );
   h3dSetNodeParamI( camera, H3DCamera::ViewportHeightI, height );
	
   // Set virtual camera parameters
   h3dSetupCameraView( camera, 45.0f, (float)width / height, 4.0f, 4000.0f);
   for (const auto& entry : pipelines_) {
      h3dResizePipelineBuffers(entry.second, width, height);
   }

   screen_resize_slot_.Signal(csg::Point2(width_, height_));
}

std::shared_ptr<RenderEntity> Renderer::CreateRenderObject(H3DNode parent, om::EntityPtr entity)
{
   std::shared_ptr<RenderEntity> result;
   dm::ObjectId id = entity->GetObjectId();
   int sid = entity->GetStoreId();

   auto& entities = entities_[sid];
   auto i = entities.find(id);

   if (i != entities.end()) {
      auto entity = i->second;
      entity->SetParent(parent);
      result = entity;
   } else {
      // LOG(WARNING) << "CREATING RENDER OBJECT " << sid << ", " << id;
      result = std::make_shared<RenderEntity>(parent, entity);
      result->FinishConstruction();
      entities[id] = result;
      traces_ += entity->TraceObjectLifetime("render entity lifetime", [=]() { 
         // LOG(WARNING) << "DESTROYING RENDER OBJECT " << sid << ", " << id;
         entities_[sid].erase(id);
      });
   }
   return result;
}

std::shared_ptr<RenderEntity> Renderer::GetRenderObject(om::EntityPtr entity)
{
   std::shared_ptr<RenderEntity> result;
   if (entity) {
      result = GetRenderObject(entity->GetStoreId(), entity->GetObjectId());
   }
   return result;
}

std::shared_ptr<RenderEntity> Renderer::GetRenderObject(int sid, dm::ObjectId id)
{
   auto i = entities_[sid].find(id);
   if (i != entities_[sid].end()) {
      return i->second;
   }
   return nullptr;
}

void Renderer::RemoveRenderObject(int sid, dm::ObjectId id)
{
   auto i = entities_[sid].find(id);
   if (i != entities_[sid].end()) {
      LOG(WARNING) << "destroying render object (sid:" << sid << " id:" << id;
      entities_[sid].erase(i);
   }
}

void Renderer::UpdateCamera()
{
   //Let the audio listener know where the camera is
   sf::Listener::setPosition(camera_->GetPosition().x, camera_->GetPosition().y, camera_->GetPosition().z);
}

void Renderer::OnMouseWheel(double value)
{
   input_.mouse.wheel = (int)value;
   CallMouseInputCallbacks();
   input_.mouse.wheel = 0;
}

void Renderer::OnMouseEnter(int entered)
{
   input_.mouse.in_client_area = entered == GL_TRUE;
   CallMouseInputCallbacks();
}

void Renderer::OnMouseMove(double x, double y)
{
   input_.mouse.dx = (int)x - input_.mouse.x;
   input_.mouse.dy = (int)y - input_.mouse.y;
   
   input_.mouse.x = (int)x;
   input_.mouse.y = (int)y;

   // xxx - this is annoying, but that's what you get... maybe revisit the
   // way we deliver mouse events and up/down tracking...
   memset(input_.mouse.up, 0, sizeof input_.mouse.up);
   memset(input_.mouse.down, 0, sizeof input_.mouse.down);

   CallMouseInputCallbacks();
}

void Renderer::OnMouseButton(int button, int press)
{
   memset(input_.mouse.up, 0, sizeof input_.mouse.up);
   memset(input_.mouse.down, 0, sizeof input_.mouse.down);

   input_.mouse.up[button] = input_.mouse.buttons[button] && (press == GLFW_RELEASE);
   input_.mouse.down[button] = !input_.mouse.buttons[button] && (press == GLFW_PRESS);
   input_.mouse.buttons[button] = (press == GLFW_PRESS);

   CallMouseInputCallbacks();
}

void Renderer::OnRawInput(UINT msg, WPARAM wParam, LPARAM lParam)
{
   input_.type = Input::RAW_INPUT;
   input_.raw_input.msg = msg;
   input_.raw_input.wp = wParam;
   input_.raw_input.lp = lParam;

   DispatchInputEvent();
}

void Renderer::DispatchInputEvent()
{
   if (input_cb_) {
      input_cb_(input_);
   }
}


void Renderer::CallMouseInputCallbacks()
{
   input_.type = Input::MOUSE;
   DispatchInputEvent();
}

void Renderer::OnKey(int key, int down)
{
   bool handled = false, uninstall = false;

   input_.type = Input::KEYBOARD;
   input_.keyboard.down = down != 0;
   input_.keyboard.key = key;

   auto isKeyDown = [](WPARAM arg) {
      return (GetKeyState(arg) & 0x8000) != 0;
   };
   DispatchInputEvent();
}

void Renderer::OnWindowResized(int newWidth, int newHeight) {
   Resize(newWidth, newHeight);
}

core::Guard Renderer::TraceSelected(H3DNode node, UpdateSelectionFn fn)
{
   ASSERT(!stdutil::contains(selectableCbs_, node));
   selectableCbs_[node] = fn;
   return core::Guard([=]() { selectableCbs_.erase(node); });
}

void Renderer::SetCurrentPipeline(std::string name)
{
   H3DRes p = 0;

   auto i = pipelines_.find(name);
   if (i == pipelines_.end()) {
	   p = h3dAddResource(H3DResTypes::Pipeline, name.c_str(), 0);
      pipelines_[name] = p;

   	LoadResources();

      // xxx - This keeps all pipeline buffers around all the time.  Should we
      // nuke all non-active pipelines so we can use their buffer memory for
      // something else (e.g. bigger shadows, better support for older hardware)
      h3dResizePipelineBuffers(p, width_, height_);
   } else {
      p = i->second;
   }
   if (p != currentPipeline_ && camera_ != NULL) {
      h3dSetNodeParamI(camera_->GetNode(), H3DCamera::PipeResI, p);
   }
   currentPipeline_ = p;
}

bool Renderer::ShouldHideRenderGrid(const csg::Point3& normal)
{
   if (viewMode_ == RPG) {
#if 0
      csg::Point3 normal(norm2d, 0);
      csg::Point3f toCamera = cameraPos_ - cameraTarget_;

      // If the normal to the render object lies in the XZ plane, we may
      // want to hide it so that objects between the camera and where we
      // think the user is looking are obscured.
      
      math2d::CoordinateSystem cs = norm2d.GetCoordinateSystem();

      if (cs == math2d::XZ_PLANE) {
         for (int i = 0; i < 3; i++) {
            if (normal[i]) {
               bool pos0 = normal[i] > 0;
               bool pos1 = toCamera[i] > 0;
               if (pos0 == pos1) {
                  return true;
               }
            }
         }
      }
#endif
   }
   return false;
}

void Renderer::SetViewMode(ViewMode mode)
{
   viewMode_ = mode;
}

core::Guard Renderer::TraceFrameStart(std::function<void()> fn)
{
   return AddTrace(renderFrameTraces_, fn);
}

core::Guard Renderer::TraceInterpolationStart(std::function<void()> fn)
{
   return AddTrace(interpolationStartTraces_, fn);
}

core::Guard Renderer::AddTrace(TraceMap& m, std::function<void()> fn)
{
   dm::TraceId tid = nextTraceId_++;
   m[tid] = fn;
   return core::Guard(std::bind(&Renderer::RemoveTrace, this, std::ref(m), tid));
}

void Renderer::RemoveTrace(TraceMap& m, dm::TraceId tid)
{
   m.erase(tid);
}

void Renderer::FireTraces(const TraceMap& m)
{
   for (const auto& entry : m) {
      entry.second();
   }
}

void Renderer::LoadResources()
{
   if (!h3dutLoadResourcesFromDisk("horde")) {
      // at this time, there's a bug in horde3d (?) which causes render
      // pipline corruption if invalid resources are even attempted to
      // load.  assert fail;
      h3dutDumpMessages();
      ASSERT(false);
   }
}

csg::Point2 Renderer::GetMousePosition() const
{
   return csg::Point2(input_.mouse.x, input_.mouse.y);
}

int Renderer::GetWidth() const
{
   return width_;
}

int Renderer::GetHeight() const
{
   return height_;
}

boost::property_tree::ptree const& Renderer::GetConfig() const
{  
   return config_;
}

void Renderer::SetScriptHost(lua::ScriptHost* scriptHost)
{
   scriptHost_ = scriptHost;
}

lua::ScriptHost* Renderer::GetScriptHost() const
{
   ASSERT(scriptHost_);
   return scriptHost_;
}

void Renderer::SetUITextureSize(int width, int height)
{
   uiWidth_ = width;
   uiHeight_ = height;

   if (uiPbo_) {
      h3dRemoveResource(uiPbo_);
      h3dReleaseUnusedResources();
   }
   if (uiTexture_) {
      h3dRemoveResource(uiTexture_);
      h3dRemoveResource(uiMatRes_);
   }
   h3dReleaseUnusedResources();

   uiPbo_ = h3dCreatePixelBuffer("screenui", width * height * 4);

   uiTexture_ = h3dCreateTexture("UI Texture", uiWidth_, uiHeight_, H3DFormats::List::TEX_BGRA8, H3DResFlags::NoTexMipmaps);
   unsigned char *data = (unsigned char *)h3dMapResStream(uiTexture_, H3DTexRes::ImageElem, 0, H3DTexRes::ImgPixelStream, false, true);
   memset(data, 0, uiWidth_ * uiHeight_ * 4);
   h3dUnmapResStream(uiTexture_);

   std::ostringstream material;
   material << "<Material>" << std::endl;
	material << "   <Shader source=\"shaders/overlay.shader\"/>" << std::endl;
	material << "   <Sampler name=\"albedoMap\" map=\"" << h3dGetResName(uiTexture_) << "\" />" << std::endl;
   material << "</Material>" << std::endl;

   uiMatRes_ = h3dAddResource(H3DResTypes::Material, "UI Material", 0);
   bool result = h3dLoadResource(uiMatRes_, material.str().c_str(), material.str().length());
   assert(result);
}

core::Guard Renderer::OnScreenResize(std::function<void(csg::Point2)> fn)
{
   return screen_resize_slot_.Register(fn);
}
