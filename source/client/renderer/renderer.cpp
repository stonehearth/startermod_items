#include "pch.h"
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


using namespace ::radiant;
using namespace ::radiant::client;

H3DNode cubemitterNode;

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
   cameraMoveDirection_(0, 0, 0),
   viewMode_(Standard),
   scriptHost_(nullptr),
   nextTraceId_(1),
   camera_(nullptr),
   currentFrameTime_(0),
   uiWidth_(0),
   uiHeight_(0),
   uiTexture_(0),
   uiMatRes_(0)

{
   try {
      boost::property_tree::json_parser::read_json("renderer_config.json", config_);
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
   
   bool vsync = true;
	glfwSwapInterval(vsync ? 0 : 1);

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
	logoMatRes_ = h3dAddResource( H3DResTypes::Material, "overlays/logo.material.xml", 0 );
	fontMatRes_ = h3dAddResource( H3DResTypes::Material, "overlays/font.material.xml", 0 );
	panelMatRes_ = h3dAddResource( H3DResTypes::Material, "overlays/panel.material.xml", 0 );


   H3DRes skyBoxRes = h3dAddResource( H3DResTypes::SceneGraph, "models/skybox/skybox.scene.xml", 0 );
   
   // xxx - should move this into the horde extension for debug shapes, but it doesn't know
   // how to actually get the resource loaded!
   h3dAddResource(H3DResTypes::Material, "materials/debug_shape.material.xml", 0); 
   
   LoadResources();

	// Add camera
   
   camera_ = new Camera(h3dAddCameraNode(H3DRootNode, "Camera", currentPipeline_));

   cameraTarget_ = csg::Point3f(0, 10, 0);

   camera_->SetPosition(csg::Point3f(24, 34, 24));
   camera_->LookAt(cameraTarget_);
   UpdateCamera();

   h3dSetNodeParamI(camera_->GetNode(), H3DCamera::PipeResI, currentPipeline_);

   // Skybox
	//H3DNode sky = h3dAddNodes( H3DRootNode, skyBoxRes );
	//h3dSetNodeTransform( sky, 128, 0, 128, 0, 0, 0, 256, 256, 256 );
	//h3dSetNodeFlags( sky, H3DNodeFlags::NoCastShadow, true );

   // Resize
   Resize(width_, height_);

   memset(&input_.mouse, 0, sizeof input_.mouse);
   rotateCamera_ = false;

   glfwSetWindowSizeCallback(window, [](GLFWwindow *window, int newWidth, int newHeight) { 
      Renderer::GetInstance().OnWindowResized(newWidth, newHeight); 
   });

   glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scancode, int action, int mods) { 
      Renderer::GetInstance().OnKey(key, (action == GLFW_PRESS) || (action == GLFW_REPEAT)); 
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

   fileWatcher_.addWatch(L"horde", [](FW::WatchID watchid, const std::wstring& dir, const std::wstring& filename, FW::Action action) -> void {
      Renderer::GetInstance().FlushMaterials();
   }, true);


   initialized_ = true;
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
   while ((r = h3dGetNextResource(RT_CubemitterResource, r)) != 0) {
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
   //return (HWND)glfwGetWindowHandle();
   return glfwGetWin32Window(glfwGetCurrentContext());
}

void Renderer::RenderOneFrame(int now, float alpha)
{
   int deltaNow = now - currentFrameTime_;

   currentFrameTime_ =  now;
   currentFrameInterp_ = alpha;

   if (cameraMoveDirection_ != csg::Point3f::zero) {
         csg::Point3f forward, up, left;
         camera_->GetBases(&forward, &up, &left);
         forward.y = 0;
         csg::Point3f camPosDelta = (left * cameraMoveDirection_.x) + (forward * cameraMoveDirection_.z);
         camPosDelta.Normalize();
         camPosDelta = camPosDelta * (deltaNow / 45.0f);
         camera_->SetPosition(camera_->GetPosition() + camPosDelta);
         cameraTarget_ += camPosDelta;
         UpdateCamera();
   }

   bool debug = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
   bool showStats = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
  
   /*if (false && (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0) {
      h3dSetNodeFlags(spotLight, 0, true);
      h3dSetNodeFlags(directionalLight, H3DNodeFlags::NoDraw, true);
   } else {
      h3dSetNodeFlags(directionalLight, 0, true);
      h3dSetNodeFlags(spotLight, H3DNodeFlags::NoDraw, true);
   }*/

   bool showUI = true;
   const float ww = (float)h3dGetNodeParamI(camera_->GetNode(), H3DCamera::ViewportWidthI) /
	                 (float)h3dGetNodeParamI(camera_->GetNode(), H3DCamera::ViewportHeightI);

   // debug = true;

	h3dSetOption(H3DOptions::DebugViewMode, debug);
	h3dSetOption(H3DOptions::WireframeMode, debug);
   // h3dSetOption(H3DOptions::DebugViewMode, _debugViewMode ? 1.0f : 0.0f);
	// h3dSetOption(H3DOptions::WireframeMode, _wireframeMode ? 1.0f : 0.0f);
	
	// Set camera parameters
	// h3dSetNodeTransform(camera_, _x, _y, _z, _rx ,_ry, 0, 1, 1, 1);
	
   h3dSetCurrentRenderTime(now / 1000.0f);

   FireTraces(renderFrameTraces_);

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

   // Show logo
   if (false) {
      const float ovLogo[] = { ww-0.4f, 0.8f, 0, 1,  ww-0.4f, 1, 0, 0,  ww, 1, 1, 0,  ww, 0.8f, 1, 1 };
      h3dShowOverlays(ovLogo, 4, 1.f, 1.f, 1.f, 1.f, logoMatRes_, 0);
   }

   if (showStats) { 
      // show stats
      h3dutShowFrameStats( fontMatRes_, panelMatRes_, H3DUTMaxStatMode );
   }
	

   fileWatcher_.update();
   LoadResources();

	// Render scene
   h3dRender(camera_->GetNode());

	// Finish rendering of frame
	h3dFinalizeFrame();
   //glFinish();

   // Advance emitter time; this must come AFTER rendering, because we only know which emitters
   // to update after doing a render pass.
   h3dRadiantAdvanceCubemitterTime(deltaNow / 1000.0f);

   // Remove all overlays
	h3dClearOverlays();

	// Write all messages to log file
	h3dutDumpMessages();
   glfwSwapBuffers(glfwGetCurrentContext());
}

bool Renderer::IsRunning() const
{
   return true;
}

csg::Ray3 Renderer::GetCameraToViewportRay(int windowX, int windowY)
{
   // compute normalized window coordinates in preparation for casting a ray
   // through the scene
   float nwx = ((float)windowX) / width_;
   float nwy = 1.0f - ((float)windowY) / height_;

   // calculate the ray starting at the eye position of the camera, casting
   // through the specified window coordinates into the scene
   csg::Ray3 ray;   

   h3dutPickRay(camera_->GetNode(), nwx, nwy,
                &ray.origin.x, &ray.origin.y, &ray.origin.z,
                &ray.direction.x, &ray.direction.y, &ray.direction.z);

   return ray;
}

void Renderer::QuerySceneRay(int windowX, int windowY, om::Selection &result)
{
   if (!rootRenderObject_) {
      return;
   }

   float nwx = ((float)windowX) / width_;
   float nwy = 1.0f - ((float)windowY) / height_;
   float ox, oy, oz, dx, dy, dz;

   h3dutPickRay(camera_->GetNode(), nwx, nwy, &ox, &oy, &oz, &dx, &dy, &dz);

	if (h3dCastRay(rootRenderObject_->GetNode(), ox, oy, oz, dx, dy, dz, 1) == 0) {
		return;
	}

   // Pull out the intersection node and intersection point
	H3DNode node = 0;
   csg::Point3f pt, normal;
	if (!h3dGetCastRayResult( 0, &node, 0, &pt.x, &normal.x )) {
      return;
   }
   normal.Normalize();
   assert(node);

   result.AddLocation(csg::Point3f(pt.x, pt.y, pt.z),
                      csg::Point3f(normal.x, normal.y, normal.z));

   // Lookup the intersection object in the node registery and ask it to
   // fill in the selection structure.
   csg::Ray3 ray(csg::Point3f(ox, oy, oz), csg::Point3f(dx, dy, dz));

   while (node) {
      const char *name = h3dGetNodeParamStr(node, H3DNodeParams::NameStr);
      auto i = selectableCbs_.find(node);
      if (i != selectableCbs_.end()) {
         // transform the interesection and the normal to the coordinate space
         // of the node.
         csg::Matrix4 transform = GetNodeTransform(node);
         transform.affine_inverse();

         i->second(result, ray, transform.transform(pt), transform.rotate(normal));
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

void Renderer::PlaceCamera(const csg::Point3f &location)
{
   csg::Point3f delta = csg::Point3f(location) - cameraTarget_;
   cameraTarget_ += delta;
   camera_->SetPosition(camera_->GetPosition() + delta);

   UpdateCamera();
}

void Renderer::UpdateUITexture(const csg::Region2& rgn, const char* buffer)
{
   if (!rgn.IsEmpty() && uiTexture_) {
      //LOG(WARNING) << "Updating " << rgn.GetArea() << " pixels from the ui texture.";

      int pitch = uiWidth_ * 4;

      char *data = (char *)h3dMapResStream(uiTexture_, H3DTexRes::ImageElem, 0, H3DTexRes::ImgPixelStream, true, true);
      if (data) {
         for (csg::Rect2 const& r : rgn) {
            int amount = (r.GetMax().x - r.GetMin().x) * 4;
            for (int y = r.GetMin().y; y < r.GetMax().y; y++) {
               char* dst = data + (y * pitch) + (r.GetMin().x * 4);         
               const char* src = buffer + (y * pitch) + (r.GetMin().x * 4);
               memcpy(dst, src, amount);
               //for (int i = 0; i < amount; i += 4) {  dst[i+3] = 0xff; }
            }
         }
         h3dUnmapResStream(uiTexture_);
      }
   }
}

void Renderer::Resize( int width, int height )
{
   width_ = width;
   height_ = height;

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
   if (screen_resize_cb_) {
      screen_resize_cb_(width_, height_);
   }
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
      if (entity->GetParent() != parent) {
         entity->SetParent(parent);
      }
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

/*
 *  gluLookAt.c
 */

void Renderer::UpdateCamera()
{
   //Let the audio listener know where the camera is
   sf::Listener::setPosition(camera_->GetPosition().x, camera_->GetPosition().y, camera_->GetPosition().z);
}

float Renderer::DistFunc(float dist, int wheel, float minDist, float maxDist) const {
   float shortFactor = 0.90f;
   float medFactor = 0.81f;
   float farFactor = 0.75f;
   float result = 0.0f;
   float factor;

   if (wheel == 0) {
      return dist;
   }

   if (dist < 100.0f) {
      factor = shortFactor;
   } else if (dist < 500.0f) {
      factor = medFactor;
   } else {
      factor = farFactor;
   }

   if (wheel > 0) {
      result = dist * factor;
   } else {
      result = dist / factor;
   }

   result = std::min(std::max(result, minDist), maxDist);

   return result;
}

void Renderer::OnMouseWheel(double value)
{
   int dWheel = (int)value;
   input_.mouse.wheel = dWheel;
   
   // xxx: move this part out into the client --
   csg::Point3f dir = camera_->GetPosition() - cameraTarget_;
   float dist = dir.Length();
   dir.Normalize();
   camera_->SetPosition(cameraTarget_ + dir * DistFunc(dist, dWheel, 10.0f, 3000.0f));

   UpdateCamera(); // xxx - defer to render time?
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

   if (rotateCamera_) {
      float degx = input_.mouse.dx / -3.0f;
      float degy = -input_.mouse.dy / 2.0f;
      camera_->OrbitPointBy(cameraTarget_, degy, degx, 10.0f, 85.0f);
      camera_->LookAt(cameraTarget_);

      UpdateCamera();
   } else {
      /*     
      int gutterSize = 10;

      if (input_.mouse.x < gutterSize) {
         cameraMoveDirection_.x = (float)-1;
      } else if (input_.mouse.x > width_ - gutterSize) {
         cameraMoveDirection_.x = (float)1;
      } else {
         cameraMoveDirection_.x = (float)0;
      }

      if (input_.mouse.y < gutterSize) {
         cameraMoveDirection_.z = (float)-1;
      } else if (input_.mouse.y > height_ - gutterSize) {
         cameraMoveDirection_.z = (float)1;
      } else {
         cameraMoveDirection_.z = 0;
      }
      */
   }

   CallMouseInputCallbacks();
}

void Renderer::OnMouseButton(int button, int press)
{
   if (initialized_) {
      if (button == GLFW_MOUSE_BUTTON_RIGHT) {
         rotateCamera_ = (press == GLFW_PRESS);
      }
   }

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
   if (!isKeyDown(VK_SHIFT)) {
      if (key == 'W') {
         cameraMoveDirection_.z = (float)(down ? -1 : 0);
      } else if (key == 'A') {
         cameraMoveDirection_.x = (float)(down ? -1 : 0);
      } else if (key == 'S') {
         cameraMoveDirection_.z = (float)(down ? 1 : 0);
      } else if (key == 'D') {
         cameraMoveDirection_.x = (float)(down ? 1 : 0);
      }
   }
   DispatchInputEvent();
}

void Renderer::OnWindowResized(int newWidth, int newHeight) {
   Resize(newWidth, newHeight);
}

dm::Guard Renderer::TraceSelected(H3DNode node, UpdateSelectionFn fn)
{
   ASSERT(!stdutil::contains(selectableCbs_, node));
   selectableCbs_[node] = fn;
   return dm::Guard([=]() { selectableCbs_.erase(node); });
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

dm::Guard Renderer::TraceFrameStart(std::function<void()> fn)
{
   return AddTrace(renderFrameTraces_, fn);
}

dm::Guard Renderer::TraceInterpolationStart(std::function<void()> fn)
{
   return AddTrace(interpolationStartTraces_, fn);
}

dm::Guard Renderer::AddTrace(TraceMap& m, std::function<void()> fn)
{
   dm::TraceId tid = nextTraceId_++;
   m[tid] = fn;
   return dm::Guard(std::bind(&Renderer::RemoveTrace, this, std::ref(m), tid));
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

   if (uiTexture_) {
      throw std::logic_error("resizing the ui texture is not yet implemented");
   }
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
