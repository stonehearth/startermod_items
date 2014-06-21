#include "pch.h"
#include "radiant.h"
#include "radiant_stdutil.h"
#include "core/config.h"
#include "core/system.h"
#include "renderer.h"
#include "Horde3DUtils.h"
#include "Horde3DRadiant.h"
#include "glfw3.h"
#include "glfw3native.h"
#include "render_entity.h"
#include "lib/perfmon/perfmon.h"
#include "dm/record_trace.h"
#include "om/selection.h"
#include "om/entity.h"
#include <SFML/Audio.hpp>
#include "camera.h"
#include "lib/perfmon/perfmon.h"
#include "perfhud/perfhud.h"
#include "resources/res_manager.h"
#include "csg/random_number_generator.h"
#include "pipeline.h"
#include <unordered_set>
#include <string>
#include "client/client.h"
#include "raycast_result.h"
#include "platform/utils.h"

using namespace ::radiant;
using namespace ::radiant::client;

#define R_LOG(level)      LOG(renderer.renderer, level)

H3DNode meshNode;
H3DNode starfieldMeshNode;
H3DRes starfieldMat;
H3DRes skysphereMat;
H3DNode cursorNode;

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
   scriptHost_(nullptr),
   camera_(nullptr),
   uiWidth_(0),
   uiHeight_(0),
   last_render_time_(0),
   server_tick_slot_("server tick"),
   render_frame_start_slot_("render frame start"),
   screen_resize_slot_("screen resize"),
   show_debug_shapes_changed_slot_("show debug shapes"),
   lastGlfwError_("none"),
   currentPipeline_(""),
   iconified_(false),
   resize_pending_(false),
   drawWorld_(true),
   last_render_time_wallclock_(0)
{
   OneTimeIninitializtion();
}

void Renderer::OneTimeIninitializtion()
{
   res::ResourceManager2::GetInstance().LookupJson("stonehearth/renderers/terrain/terrain_renderer.json", [&](const json::Node& n) {
      terrainConfig_ = n;
   });
   GetConfigOptions();

   assert(renderer_.get() == nullptr);
   renderer_.reset(this);

   InitWindow();
   InitHorde();
   SetupGlfwHandlers();
   MakeRendererResources();

   ApplyConfig(config_, false);
   LoadResources();

   memset(&input_.mouse, 0, sizeof input_.mouse);
   input_.focused = true;

   // If the mod is unzipped, put a watch on the filesystem directory where the resources live
   // so we can dynamically load resources whenever the file changes.
   std::string fspath = std::string("mods/") + resourcePath_;
   if (boost::filesystem::is_directory(fspath)) {
      fileWatcher_.addWatch(strutil::utf8_to_unicode(fspath), [](FW::WatchID watchid, const std::wstring& dir, const std::wstring& filename, FW::Action action) -> void {
         Renderer::GetInstance().FlushMaterials();
      }, true);
   }

   OnWindowResized(windowWidth_, windowHeight_);
   SetShowDebugShapes(false);

   SetDrawWorld(false);
   initialized_ = true;
}

void Renderer::MakeRendererResources()
{
   // Overlays
   fontMatRes_ = h3dAddResource( H3DResTypes::Material, "overlays/font.material.xml", 0 );
   panelMatRes_ = h3dAddResource( H3DResTypes::Material, "overlays/panel.material.xml", 0 );

   H3DRes veclookup = h3dCreateTexture("RandomVectorLookup", 4, 4, H3DFormats::TEX_RGBA32F, H3DResFlags::NoTexMipmaps | H3DResFlags::NoQuery | H3DResFlags::NoFlush);

   csg::RandomNumberGenerator &rng = csg::RandomNumberGenerator::DefaultInstance();
   float *data2 = (float *)h3dMapResStream(veclookup, H3DTexRes::ImageElem, 0, H3DTexRes::ImgPixelStream, false, true);
   for (int i = 0; i < 16; i++)
   {
      float x = rng.GetReal(-1.0f, 1.0f);
      float y = rng.GetReal(-1.0f, 1.0f);
      float z = rng.GetReal(-1.0f, 1.0f);
      Horde3D::Vec3f v(x,y,z);
      v.normalize();

      data2[(i * 4) + 0] = v.x;
      data2[(i * 4) + 1] = v.y;
      data2[(i * 4) + 2] = v.z;
   }
   h3dUnmapResStream(veclookup);

   fowRenderTarget_ = h3dutCreateRenderTarget(512, 512, H3DFormats::TEX_BGRA8, false, 1, 0, 0);
}

void Renderer::InitHorde()
{
   GLFWwindow* window = glfwGetCurrentContext();
   int numWindowSamples = glfwGetWindowAttrib(window, GLFW_SAMPLES);
   // Init Horde, looking for OpenGL 2.0 minimum.
   std::string s = (radiant::core::System::GetInstance().GetTempDirectory() / "gfx.log").string();
   if (!h3dInit(2, 0, numWindowSamples > 0, config_.enable_gl_logging.value, s.c_str())) {
      h3dutDumpMessages();
      throw std::runtime_error("Unable to initialize renderer.  Check horde log for details.");
   }

   // Set options
   h3dSetOption(H3DOptions::LoadTextures, 1);
   h3dSetOption(H3DOptions::TexCompression, 0);
   h3dSetOption(H3DOptions::MaxAnisotropy, 1);
   h3dSetOption(H3DOptions::FastAnimation, 1);
   h3dSetOption(H3DOptions::DumpFailedShaders, 1);
}

void Renderer::InitWindow()
{
   glfwSetErrorCallback([](int errorCode, const char* errorString) {
      Renderer::GetInstance().lastGlfwError_ = BUILD_STRING(errorString << " (code: " << std::to_string(errorCode) << ")");
   });

   if (!glfwInit())
   {
      throw std::runtime_error(BUILD_STRING("Unable to initialize glfw: " << lastGlfwError_));
   }

   inFullscreen_ = config_.enable_fullscreen.value;
   GLFWmonitor* monitor;
   int windowX, windowY;
   SelectSaneVideoMode(config_.enable_fullscreen.value, &windowWidth_, &windowHeight_, &windowX, &windowY, &monitor);

   if (!inFullscreen_) {
      config_.last_window_x.value = windowX;
      config_.last_window_y.value = windowY;
      config_.screen_height.value = windowHeight_;
      config_.screen_width.value = windowWidth_;
   }

   glfwWindowHint(GLFW_SAMPLES, config_.num_msaa_samples.value);
   glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, config_.enable_gl_logging.value ? 1 : 0);

   GLFWwindow *window;
   if (!(window = glfwCreateWindow(windowWidth_, windowHeight_, "Stonehearth", 
        config_.enable_fullscreen.value ? monitor : nullptr, nullptr))) {
      glfwTerminate();
      throw std::runtime_error(BUILD_STRING("Unable to create glfw window: " << lastGlfwError_));
   }

   glfwMakeContextCurrent(window);
   glfwGetWindowSize(window, &windowWidth_, &windowHeight_);
   if (!inFullscreen_) {
      SetWindowPos(GetWindowHandle(), NULL, windowX, windowY, 0, 0, SWP_NOSIZE);
   }

   if (config_.minimized.value) {
      glfwIconifyWindow(window);
   }
}

void Renderer::SetupGlfwHandlers()
{
   GLFWwindow *window = glfwGetCurrentContext();
   glfwSetWindowSizeCallback(window, [](GLFWwindow *window, int newWidth, int newHeight) { 
      Renderer::GetInstance().resize_pending_ = true;
      Renderer::GetInstance().nextHeight_ = newHeight;
      Renderer::GetInstance().nextWidth_ = newWidth;
   });

   glfwSetWindowIconifyCallback(window, [](GLFWwindow *window, int iconified) {
      Renderer::GetInstance().iconified_ = iconified == GL_TRUE;
   });

   glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scancode, int action, int mods) { 
      Renderer::GetInstance().OnKey(key, (action == GLFW_PRESS) || (action == GLFW_REPEAT)); 
   });

   glfwSetCursorEnterCallback(window, [](GLFWwindow *window, int entered) {
      Renderer::GetInstance().OnMouseEnter(entered);
   });

   glfwSetWindowFocusCallback(window, [](GLFWwindow *window, int wasFocused) {
      Renderer::GetInstance().OnFocusChanged(wasFocused);
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
      // All Win32 specific!

      // This nonsense is because the upper-left corner of the window (on a multi-monitor system)
      // will actually bleed into the adjacent window, so we need to figure out what kind of bias
      // that needs to be _in_ the monitor.  We get that bias by calling ScreenToClient on the
      // upper-left of the window's rect.
      RECT rect;
      GetWindowRect(Renderer::GetInstance().GetWindowHandle(), &rect);

      POINT p; p.x = 0; p.y = 0;
      POINT p2; p2.x = rect.left; p2.y = rect.top;
      ScreenToClient(Renderer::GetInstance().GetWindowHandle(), &p2);
      ClientToScreen(Renderer::GetInstance().GetWindowHandle(), &p);

      if (!Renderer::GetInstance().inFullscreen_) {
         Renderer::GetInstance().config_.last_window_x.value = (int)rect.left;
         Renderer::GetInstance().config_.last_window_y.value = (int)rect.top;

         Renderer::GetInstance().config_.last_screen_x.value = p.x - p2.x;
         Renderer::GetInstance().config_.last_screen_y.value = p.y - p2.y;

         Renderer::GetInstance().config_.screen_width.value = Renderer::GetInstance().windowWidth_;
         Renderer::GetInstance().config_.screen_height.value = Renderer::GetInstance().windowHeight_;
      }
      Renderer::GetInstance().ApplyConfig(Renderer::GetInstance().config_, true);

      R_LOG(0) << "window closed.  exiting process";
      TerminateProcess(GetCurrentProcess(), 1);
   });
}

void Renderer::UpdateFoW(H3DNode node, const csg::Region2& region)
{
   if (region.GetCubeCount() >= 1000) {
      return;
   }
   float* start = (float*)h3dMapNodeParamV(node, H3DInstanceNodeParams::InstanceBuffer);
   float* f = start;
   float ySize = 100.0f;
   for (const auto& c : region) 
   {
      float px = (c.max + c.min).x * 0.5f;
      float pz = (c.max + c.min).y * 0.5f;

      float xSize = (float)c.max.x - c.min.x;
      float zSize = (float)c.max.y - c.min.y;
      f[0] = xSize; f[1] = 0;                f[2] =   0;    f[3] =  0;
      f[4] = 0;     f[5] = ySize;            f[6] =   0;    f[7] =  0;
      f[8] = 0;     f[9] = 0;                f[10] = zSize; f[11] = 0;
      f[12] = px;   f[13] = -ySize/2.0f;     f[14] =  pz;   f[15] = 1;
      
      f += 16;
   }

   h3dUnmapNodeParamV(node, H3DInstanceNodeParams::InstanceBuffer, (f - start) * 4);
   const auto& bounds = region.GetBounds();
   h3dUpdateBoundingBox(node, (float)bounds.min.x, -ySize/2.0f, (float)bounds.min.y, 
      (float)bounds.max.x, ySize/2.0f, (float)bounds.max.y);   
}

void Renderer::RenderFogOfWarRT()
{
   if (!camera_) {
      return;
   }

   // Create an ortho camera covering the largest volume that can be seen of the actual possible frustum
   Horde3D::Matrix4f fowView, camProj;
   Horde3D::Frustum camFrust;

   h3dGetCameraFrustum(camera_->GetNode(), &camFrust);

   // Construct a new camera view matrix pointing down.
   fowView.x[0] = 1;  fowView.x[1] = 0;  fowView.x[2] = 0;
   fowView.x[4] = 0;  fowView.x[5] = 0;  fowView.x[6] = 1;
   fowView.x[8] = 0;  fowView.x[9] = -1;  fowView.x[10] = 0;
   fowView.x[12] = 0; fowView.x[13] = 0; fowView.x[14] = 0;  fowView.x[15] = 1.0;

   Horde3D::BoundingBox bb;
   for (int i = 0; i < 8; i++) {
      bb.addPoint(fowView * camFrust.getCorner(i));
   }

   float cascadeBound = (camFrust.getCorner(0) - camFrust.getCorner(6)).length();
   Horde3D::Vec3f boarderOffset = (Horde3D::Vec3f(cascadeBound, cascadeBound, cascadeBound) - (bb.max() - bb.min())) * 0.5f;
   boarderOffset.z = 0;
   Horde3D::Vec3f max = bb.max() + boarderOffset;
   Horde3D::Vec3f min = bb.min() - boarderOffset;

   // The world units per texel are used to snap the shadow the orthographic projection
   // to texel sized increments.  This keeps the edges of the shadows from shimmering.
   float worldUnitsPerTexel = cascadeBound / 512.0f;
   Horde3D::Vec3f quantizer(worldUnitsPerTexel, worldUnitsPerTexel, 0.0);
   min.quantize(quantizer);
   max.quantize(quantizer);

   // All that crap was just so we could set up this ortho frustum + view matrix.
   h3dSetNodeTransMat(fowCamera_->GetNode(), fowView.inverted().x);
   h3dSetNodeParamI(fowCamera_->GetNode(), H3DCamera::OrthoI, 1);
   h3dSetNodeParamF(fowCamera_->GetNode(), H3DCamera::LeftPlaneF, 0, min.x);
   h3dSetNodeParamF(fowCamera_->GetNode(), H3DCamera::RightPlaneF, 0, max.x);
   h3dSetNodeParamF(fowCamera_->GetNode(), H3DCamera::BottomPlaneF, 0, min.y);
   h3dSetNodeParamF(fowCamera_->GetNode(), H3DCamera::TopPlaneF, 0, max.y);
   h3dSetNodeParamF(fowCamera_->GetNode(), H3DCamera::NearPlaneF, 0, -max.z);
   h3dSetNodeParamF(fowCamera_->GetNode(), H3DCamera::FarPlaneF, 0, -min.z);

   H3DRes fp = GetPipeline("pipelines/fow.pipeline.xml");
   h3dSetResParamStr(fp, H3DPipeRes::GlobalRenderTarget, 0, fowRenderTarget_, "FogOfWarRT");

   h3dRender(fowCamera_->GetNode(), fp);

   Horde3D::Matrix4f pm;
   h3dGetCameraProjMat(fowCamera_->GetNode(), pm.x);

   Horde3D::Matrix4f c = pm * fowView;
   float m[] = {
      0.5, 0.0, 0.0, 0.0,
      0.0, 0.5, 0.0, 0.0,
      0.0, 0.0, 0.5, 0.0,
      0.5, 0.5, 0.5, 1.0
   };
   Horde3D::Matrix4f biasMatrix(m);
   c = biasMatrix * c;

   h3dSetGlobalUniform("fowViewMat", H3DUniformType::MAT44, c.x);
}

void Renderer::SetSkyColors(const csg::Point3f& startCol, const csg::Point3f& endCol)
{
   h3dSetMaterialUniform(skysphereMat, "skycolor_start", startCol.x, startCol.y, startCol.z, 1.0f);
   h3dSetMaterialUniform(skysphereMat, "skycolor_end", endCol.x, endCol.y, endCol.z, 1.0f);
}

H3DRes Renderer::BuildSphereGeometry() {
   float texData[2046 * 2];
   // Unit (geodesic) sphere (created by recursively subdividing a base octahedron)
   Horde3D::Vec3f spVerts[2046] = {  // x, y, z
      Horde3D::Vec3f( 0.f, 1.f, 0.f ),   Horde3D::Vec3f( 0.f, -1.f, 0.f ),
      Horde3D::Vec3f( -0.707f, 0.f, 0.707f ),   Horde3D::Vec3f( 0.707f, 0.f, 0.707f ),
      Horde3D::Vec3f( 0.707f, 0.f, -0.707f ),   Horde3D::Vec3f( -0.707f, 0.f, -0.707f )
   };
   uint32 spInds[2048 * 3] = {  // Number of faces: (4 ^ iterations) * 8
      2, 3, 0,   3, 4, 0,   4, 5, 0,   5, 2, 0,   2, 1, 3,   3, 1, 4,   4, 1, 5,   5, 1, 2
   };
   for( uint32 i = 0, nv = 6, ni = 24; i < 4; ++i )  // Two iterations
   {
      // Subdivide each face into 4 tris by bisecting each edge and push vertices onto unit sphere
      for( uint32 j = 0, prevNumInds = ni; j < prevNumInds; j += 3 )
      {
         spVerts[nv++] = ((spVerts[spInds[j + 0]] + spVerts[spInds[j + 1]]) * 0.5f).normalized();
         spVerts[nv++] = ((spVerts[spInds[j + 1]] + spVerts[spInds[j + 2]]) * 0.5f).normalized();
         spVerts[nv++] = ((spVerts[spInds[j + 2]] + spVerts[spInds[j + 0]]) * 0.5f).normalized();

         spInds[ni++] = spInds[j + 0]; spInds[ni++] = nv - 3; spInds[ni++] = nv - 1;
         spInds[ni++] = nv - 3; spInds[ni++] = spInds[j + 1]; spInds[ni++] = nv - 2;
         spInds[ni++] = nv - 2; spInds[ni++] = spInds[j + 2]; spInds[ni++] = nv - 1;
         spInds[j + 0] = nv - 3; spInds[j + 1] = nv - 2; spInds[j + 2] = nv - 1;
      }
   }

   for (int i = 0; i < 2046; i++)
   {
      texData[i * 2 + 0] = 0.0f;
      texData[i * 2 + 1] = 1.0f - ((spVerts[i].y * 0.5f) + 0.5f);
   }

   return h3dutCreateGeometryRes("sphere", 2046, 2048 * 3, (float*)spVerts, spInds, nullptr, nullptr, nullptr, texData, nullptr);
}

void Renderer::BuildSkySphere()
{
   float texData[2046 * 2];
   // Unit (geodesic) sphere (created by recursively subdividing a base octahedron)
   Horde3D::Vec3f spVerts[2046] = {  // x, y, z
      Horde3D::Vec3f( 0.f, 1.f, 0.f ),   Horde3D::Vec3f( 0.f, -1.f, 0.f ),
      Horde3D::Vec3f( -0.707f, 0.f, 0.707f ),   Horde3D::Vec3f( 0.707f, 0.f, 0.707f ),
      Horde3D::Vec3f( 0.707f, 0.f, -0.707f ),   Horde3D::Vec3f( -0.707f, 0.f, -0.707f )
   };
   uint32 spInds[2048 * 3] = {  // Number of faces: (4 ^ iterations) * 8
      2, 3, 0,   3, 4, 0,   4, 5, 0,   5, 2, 0,   2, 1, 3,   3, 1, 4,   4, 1, 5,   5, 1, 2
   };
   for( uint32 i = 0, nv = 6, ni = 24; i < 4; ++i )  // Two iterations
   {
      // Subdivide each face into 4 tris by bisecting each edge and push vertices onto unit sphere
      for( uint32 j = 0, prevNumInds = ni; j < prevNumInds; j += 3 )
      {
         spVerts[nv++] = ((spVerts[spInds[j + 0]] + spVerts[spInds[j + 1]]) * 0.5f).normalized();
         spVerts[nv++] = ((spVerts[spInds[j + 1]] + spVerts[spInds[j + 2]]) * 0.5f).normalized();
         spVerts[nv++] = ((spVerts[spInds[j + 2]] + spVerts[spInds[j + 0]]) * 0.5f).normalized();

         spInds[ni++] = spInds[j + 0]; spInds[ni++] = nv - 3; spInds[ni++] = nv - 1;
         spInds[ni++] = nv - 3; spInds[ni++] = spInds[j + 1]; spInds[ni++] = nv - 2;
         spInds[ni++] = nv - 2; spInds[ni++] = spInds[j + 2]; spInds[ni++] = nv - 1;
         spInds[j + 0] = nv - 3; spInds[j + 1] = nv - 2; spInds[j + 2] = nv - 1;
      }
   }

   // Set up texture coordinates for each vertex based on it's 'height'.  We will use this to
   // interpolate a gradient.
   for (int i = 0; i < 2046; i++)
   {
      texData[i * 2 + 0] = 0.0f;
      texData[i * 2 + 1] = 1.0f - ((spVerts[i].y * 0.5f) + 0.5f);
   }

   skysphereMat = h3dAddResource(H3DResTypes::Material, "materials/skysphere.material.xml", 0);
   H3DRes geoRes = h3dutCreateGeometryRes("skysphere", 2046, 2048 * 3, (float*)spVerts, spInds, nullptr, nullptr, nullptr, texData, nullptr);
   H3DNode modelNode = h3dAddModelNode(H3DRootNode, "skysphere_model", geoRes);
   meshNode = h3dAddMeshNode(modelNode, "skysphere_mesh", skysphereMat, 0, 2048 * 3, 0, 2045);
   h3dSetNodeFlags(modelNode, H3DNodeFlags::NoCastShadow | H3DNodeFlags::NoRayQuery, true);
}

void Renderer::SetStarfieldBrightness(float brightness)
{
   h3dSetMaterialUniform(starfieldMat, "brightness", brightness, brightness, brightness, brightness);
}

void Renderer::BuildStarfield()
{
   const int NumStars = 2000;

   Horde3D::Vec3f* verts = new Horde3D::Vec3f[4 * NumStars];
   uint32* indices = new uint32[6 * NumStars];
   float* texCoords = new float[4 * 2 * NumStars];
   float* texCoords2 = new float[4 * 2 * NumStars];

   csg::RandomNumberGenerator rng(123);
   for (int i = 0; i < NumStars * 4; i+=4)
   {
      Horde3D::Vec3f starPos(
         rng.GetReal(-1.0f, 1.0f), 
         rng.GetReal(-0.8f, 0.1f), 
         rng.GetReal(-1.0f, 1.0f));
      starPos.normalize();
      verts[i + 0] = starPos;
      verts[i + 1] = starPos;
      verts[i + 2] = starPos;
      verts[i + 3] = starPos;
   }

   for (int i = 0; i < NumStars * 4 * 2; i+=8)
   {
      int size = rng.GetInt(1, 2);
      texCoords[i + 0] = 0; texCoords[i + 1] = 0;
      texCoords[i + 2] = 2.0f * size; texCoords[i + 3] = 0;
      texCoords[i + 4] = 2.0f * size; texCoords[i + 5] = 2.0f * size;
      texCoords[i + 6] = 0; texCoords[i + 7] = 2.0f * size;

      float brightness = rng.GetInt(0, 1) == 0 ? 1.0f : 0.3f;
      texCoords2[i + 0] = brightness; texCoords2[i + 1] = brightness;
      texCoords2[i + 2] = brightness; texCoords2[i + 3] = brightness;
      texCoords2[i + 4] = brightness; texCoords2[i + 5] = brightness;
      texCoords2[i + 6] = brightness; texCoords2[i + 7] = brightness;
   }

   for (int i = 0, v = 0; i < NumStars * 6; i+=6, v += 4)
   {
      indices[i + 0] = v; indices[i + 1] = v + 1; indices[i + 2] = v + 2;
      indices[i + 3] = v; indices[i + 4] = v + 2; indices[i + 5] = v + 3;
   }

   starfieldMat = h3dAddResource(H3DResTypes::Material, "materials/starfield.material.xml", 0);

   H3DRes geoRes = h3dutCreateGeometryRes("starfield", NumStars * 4, NumStars * 6, (float*)verts, indices, nullptr, nullptr, nullptr, texCoords, texCoords2);
   
   H3DNode modelNode = h3dAddModelNode(H3DRootNode, "starfield_model", geoRes);
   starfieldMeshNode = h3dAddMeshNode(modelNode, "starfield_mesh", starfieldMat, 0, NumStars * 6, 0, NumStars * 4 - 1);
   h3dSetNodeFlags(modelNode, H3DNodeFlags::NoCastShadow | H3DNodeFlags::NoRayQuery, true);
}

void Renderer::ShowPerfHud(bool value) {
   if (value && !perf_hud_) {
      perf_hud_.reset(new PerfHud(*this));
   } else if (!value && perf_hud_) {
      delete perf_hud_.release();
   }
}

// These options should be worded so that they can default to false
void Renderer::GetConfigOptions()
{
   const core::Config& config = core::Config::GetInstance();

   config_.enable_ssao.value = config.Get("renderer.enable_ssao", false);

   // "Enables shadows."
   config_.use_shadows.value = config.Get("renderer.enable_shadows", true);

   // "Sets the number of Multi-Sample Anti Aliasing samples to use."
   config_.num_msaa_samples.value = config.Get("renderer.msaa_samples", 0);

   // "Sets the square resolution of the shadow maps."
   config_.shadow_resolution.value = config.Get("renderer.shadow_resolution", 2048);

   // "Enables vertical sync."
   config_.enable_vsync.value = config.Get("renderer.enable_vsync", true);

   config_.enable_fullscreen.value = config.Get("renderer.enable_fullscreen", false);

   config_.enable_gl_logging.value = config.Get("renderer.enable_gl_logging", false);

   config_.screen_width.value = config.Get("renderer.screen_width", 1280);
   config_.screen_height.value = config.Get("renderer.screen_height", 720);

   config_.enable_debug_keys.value = config.Get("enable_debug_keys", false);

   config_.draw_distance.value = config.Get("renderer.draw_distance", 1000.0f);

   config_.last_window_x.value = config.Get("renderer.last_window_x", 0);
   config_.last_window_y.value = config.Get("renderer.last_window_y", 0);

   config_.last_screen_x.value = config.Get("renderer.last_screen_x", 0);
   config_.last_screen_y.value = config.Get("renderer.last_screen_y", 0);

   config_.use_fast_hilite.value = config.Get("renderer.use_fast_hilite", false);

   config_.minimized.value = config.Get("renderer.minimized", false);
   
   _maxRenderEntityLoadTime = core::Config::GetInstance().Get<int>("max_render_entity_load_time", 50);

   resourcePath_ = config.Get("renderer.resource_path", "stonehearth/data/horde");
}

void Renderer::UpdateConfig(const RendererConfig& newConfig)
{
   memcpy(&config_, &newConfig, sizeof(RendererConfig));

   H3DRendererCaps rendererCaps;
   H3DGpuCaps gpuCaps;
   h3dGetCapabilities(&rendererCaps, &gpuCaps);

   // Presently, only the engine can decide if certain features are even allowed to run.
   config_.num_msaa_samples.allowed = gpuCaps.MSAASupported;
   config_.use_shadows.allowed = rendererCaps.ShadowsSupported;
   config_.enable_ssao.allowed = rendererCaps.SsaoSupported;
}

void Renderer::PersistConfig()
{
   core::Config& config = core::Config::GetInstance();

   config.Set("renderer.enable_ssao", config_.enable_ssao.value);

   config.Set("renderer.enable_shadows", config_.use_shadows.value);
   config.Set("renderer.msaa_samples", config_.num_msaa_samples.value);

   config.Set("renderer.shadow_resolution", config_.shadow_resolution.value);

   config.Set("renderer.enable_vsync", config_.enable_vsync.value);

   config.Set("renderer.enable_fullscreen", config_.enable_fullscreen.value);

   config.Set("renderer.screen_width", config_.screen_width.value);
   config.Set("renderer.screen_height", config_.screen_height.value);
   config.Set("renderer.draw_distance", config_.draw_distance.value);

   config.Set("renderer.last_window_x", config_.last_window_x.value);
   config.Set("renderer.last_window_y", config_.last_window_y.value);
   config.Set("renderer.use_fast_hilite", config_.use_fast_hilite.value);

   config.Set("renderer.last_screen_x", config_.last_screen_x.value);
   config.Set("renderer.last_screen_y", config_.last_screen_y.value);
}

void Renderer::ApplyConfig(const RendererConfig& newConfig, bool persistConfig)
{
   UpdateConfig(newConfig);

   config_.enable_ssao.value &= config_.enable_ssao.allowed;
   // Super hard-coded setting for now.
   if (config_.enable_ssao.value) {
      worldPipeline_ = "pipelines/forward_postprocess.pipeline.xml";
   } else {
      worldPipeline_ = "pipelines/forward.pipeline.xml";
   }

   int oldMSAACount = (int)h3dGetOption(H3DOptions::SampleCount);

   h3dSetOption(H3DOptions::EnableShadows, config_.use_shadows.value ? 1.0f : 0.0f);
   h3dSetOption(H3DOptions::ShadowMapSize, (float)config_.shadow_resolution.value);
   h3dSetOption(H3DOptions::SampleCount, (float)config_.num_msaa_samples.value);

   /* Unused, until we can reload the window without bringing everything down.
   if (oldMSAACount != (int)h3dGetOption(H3DOptions::SampleCount))
   {
      // MSAA change requires that we reload our pipelines (so that we can regenerate our
      // render target textures with the appropriate sampling).
      FlushMaterials();

      LoadResources();
   }*/

   // Propagate far-plane value.
   ResizeViewport();

   SetStageEnable(GetPipeline(worldPipeline_), "Selected_Fast", config_.use_fast_hilite.value);
   SetStageEnable(GetPipeline(worldPipeline_), "Selected", !config_.use_fast_hilite.value);

   glfwSwapInterval(config_.enable_vsync.value ? 1 : 0);

   if (persistConfig) {
      PersistConfig();
   }
}

void Renderer::SelectSaneVideoMode(bool fullscreen, int* width, int* height, int* windowX, int* windowY, GLFWmonitor** monitor) 
{
   *windowX = config_.last_window_x.value;
   *windowY = config_.last_window_y.value;

   int last_res_width = config_.screen_width.value;
   int last_res_height = config_.screen_height.value;

   if (!fullscreen) {
      // If we're not fullscreen, then just ensure the size of the window <= the res of the monitor.
      *monitor = glfwGetPrimaryMonitor();
      const GLFWvidmode* m = glfwGetVideoMode(*monitor);
      *width = std::max(320, std::min(last_res_width, m->width));
      *height = std::max(240, std::min(last_res_height, m->height));
   } else {
      // In fullscreen, try to find the monitor that contains the window's upper-left coordinate.
      int lastX = config_.last_screen_x.value;
      int lastY = config_.last_screen_y.value;
      int numMonitors;
      int monitorWidth;
      int monitorHeight;
      GLFWmonitor** monitors = glfwGetMonitors(&numMonitors);
      GLFWmonitor *desiredMonitor = NULL;
      for (int i = 0; i < numMonitors; i++) 
      {
         int monitorX, monitorY;
         glfwGetMonitorPos(monitors[i], &monitorX, &monitorY);
         const GLFWvidmode* m = glfwGetVideoMode(monitors[i]);
      
         if ((lastX >= monitorX && lastY >= monitorY) && 
            (lastX < monitorX + m->width && lastY < monitorY + m->height)) 
         {
            desiredMonitor = monitors[i];
            monitorWidth = m->width;
            monitorHeight = m->height;
            break;
         }
      }

      if (desiredMonitor == NULL) {
         desiredMonitor = glfwGetPrimaryMonitor();
         const GLFWvidmode* m = glfwGetVideoMode(desiredMonitor);
         monitorWidth = m->width;
         monitorHeight = m->height;
      }
      *monitor = desiredMonitor;
      *width = monitorWidth;
      *height = monitorHeight;
   }
}

SystemStats Renderer::GetStats()
{
   SystemStats result;

   result.frame_rate = 1000.0f / h3dGetStat(H3DStats::AverageFrameTime, false);

   result.gpu_vendor = (char *)glGetString( GL_VENDOR );
   result.gpu_renderer = (char *)glGetString( GL_RENDERER );
   result.gl_version = (char *)glGetString( GL_VERSION );

   int CPUInfo[5] = { 0 };
   __cpuid(CPUInfo, 0x80000000);
   unsigned int nExIds = CPUInfo[0];

   std::ostringstream info;
   for (uint i = 0x80000002; i <= std::min(0x80000004, nExIds); i++) {
      __cpuid(CPUInfo, i);
      info << (char *)(CPUInfo);
   }
   result.cpu_info = info.str();

#ifdef _WIN32
   MEMORYSTATUSEX status;
   status.dwLength = sizeof(status);
   GlobalMemoryStatusEx(&status);
   result.memory_gb = (int)(status.ullTotalPhys / 1048576.0f);
#endif

   R_LOG(3) << "reported fps: " << result.frame_rate;
   return result;
}

void Renderer::SetStageEnable(H3DRes pipeRes, const char* stageName, bool enabled)
{
   int stageCount = h3dGetResElemCount(pipeRes, H3DPipeRes::StageElem);

   for (int i = 0; i < stageCount; i++)
   {
      const char* curStageName = h3dGetResParamStr(pipeRes, H3DPipeRes::StageElem, i, H3DPipeRes::StageNameStr);
      if (_strcmpi(stageName, curStageName) == 0)
      {
         h3dSetResParamI(pipeRes, H3DPipeRes::StageElem, i, H3DPipeRes::StageActivationI, enabled ? 1 : 0);
         break;
      }
   }
}

void Renderer::FlushMaterials() {
   H3DRes r = 0;
   while ((r = h3dGetNextResource(H3DResTypes::Shader, r)) != 0) {
      if (!(h3dGetResFlags(r) & H3DResFlags::NoFlush)) {
         h3dUnloadResource(r);
      }
   }

   r = 0;
   while ((r = h3dGetNextResource(H3DResTypes::Material, r)) != 0) {
      if (!(h3dGetResFlags(r) & H3DResFlags::NoFlush)) {
         h3dUnloadResource(r);
      }
   }

   r = 0;
   while ((r = h3dGetNextResource(H3DResTypes::Pipeline, r)) != 0) {
      if (!(h3dGetResFlags(r) & H3DResFlags::NoFlush)) {
         h3dUnloadResource(r);
      }
   }

   r = 0;
   while ((r = h3dGetNextResource(H3DResTypes::ParticleEffect, r)) != 0) {
      if (!(h3dGetResFlags(r) & H3DResFlags::NoFlush)) {
         h3dUnloadResource(r);
      }
   }

   r = 0;
   while ((r = h3dGetNextResource(H3DResTypes::Code, r)) != 0) {
      if (!(h3dGetResFlags(r) & H3DResFlags::NoFlush)) {
         h3dUnloadResource(r);
      }
   }

   r = 0;
   while ((r = h3dGetNextResource(RT_CubemitterResource, r)) != 0) {
      if (!(h3dGetResFlags(r) & H3DResFlags::NoFlush)) {
         h3dUnloadResource(r);
      }
   }

   r = 0;
   while ((r = h3dGetNextResource(RT_AnimatedLightResource, r)) != 0) {
      if (!(h3dGetResFlags(r) & H3DResFlags::NoFlush)) {
         h3dUnloadResource(r);
      }
   }

   ResizePipelines();
}

Renderer::~Renderer()
{
   glfwDestroyWindow(glfwGetCurrentContext());
   glfwTerminate();
}

void Renderer::SetRootEntity(om::EntityPtr rootObject)
{
   rootRenderObject_ = CreateRenderEntity(H3DRootNode, rootObject);
}

void Renderer::Initialize()
{
   RenderNode::Initialize();

   BuildSkySphere();
   BuildStarfield();

   csg::Region3::Cube littleCube(csg::Region3::Point(0, 0, 0), csg::Region3::Point(1, 1, 1));
   /*fowVisibleNode_ = h3dAddInstanceNode(H3DRootNode, "fow_visiblenode", 
      h3dAddResource(H3DResTypes::Material, "materials/fow_visible.material.xml", 0), 
      Pipeline::GetInstance().CreateVoxelGeometryFromRegion("littlecube", littleCube), 1000);*/
   fowExploredNode_ = h3dAddInstanceNode(H3DRootNode, "fow_explorednode", 
      h3dAddResource(H3DResTypes::Material, "materials/fow_explored.material.xml", 0), 
      Pipeline::GetInstance().CreateVoxelGeometryFromRegion("littlecube", littleCube), 1000);

   csg::Region2 r;
   r.Add(csg::Rect2(csg::Region2::Point(-1000, -1000), csg::Region2::Point(1000, 1000)));
   UpdateFoW(fowExploredNode_, r);

   // Add camera   
   camera_ = new Camera(H3DRootNode, "Camera");

   // Add another camera--this is exclusively for the fog-of-war pipeline.
   fowCamera_ = new Camera(H3DRootNode, "FowCamera");

   debugShapes_ = h3dRadiantAddDebugShapes(H3DRootNode, "renderer debug shapes");

   // We now have a camera, so update our viewport, and inform all interested parties of
   // the update.
   ResizeViewport();
   CallWindowResizeListeners();

   last_render_time_wallclock_ = platform::get_current_time_in_ms();
}

void Renderer::Shutdown()
{
   RenderNode::Shutdown();

   show_debug_shapes_changed_slot_.Clear();
   server_tick_slot_.Clear();
   render_frame_start_slot_.Clear();

   exploredTrace_ = nullptr;
   visibilityTrace_ = nullptr;
   rootRenderObject_ = nullptr;
   for (auto& e : entities_) {
      e.clear();
   }

   debugShapes_ = 0;
   fowExploredNode_ = 0;
   delete camera_;      camera_ = nullptr;
   delete fowCamera_;   fowCamera_ = nullptr;
   h3dReset();
   
   ASSERT(RenderEntity::GetTotalObjectCount() == 0);
}

void Renderer::SetServerTick(int tick)
{
   perfmon::TimelineCounterGuard tcg("signal server tick");
   server_tick_slot_.Signal(tick);
}

void Renderer::DecodeDebugShapes(const ::radiant::protocol::shapelist& msg)
{
   h3dRadiantDecodeDebugShapes(debugShapes_, msg);
}

HWND Renderer::GetWindowHandle() const
{
   return glfwGetWin32Window(glfwGetCurrentContext());
}

bool Renderer::SetVisibleRegion(std::string const& visible_region_uri)
{
   dm::Store const& store = Client::GetInstance().GetStore();
   om::Region2BoxedPtr visibleRegionBoxed = store.FetchObject<om::Region2Boxed>(visible_region_uri);

   if (visibleRegionBoxed) {
      visibilityTrace_ = visibleRegionBoxed->TraceChanges("render visible region", dm::RENDER_TRACES)
                            ->OnModified([=](){
                               csg::Region2 visibleRegion = visibleRegionBoxed->Get();
                               // TODO: give visibleRegion to horde
                               //int num_cubes = visibleRegion.GetCubeCount();
                               //R_LOG(3) << "Client visibility cubes: " << num_cubes;
                            })
                            ->PushObjectState(); // Immediately send the current state to listener
      return true;
   }
   R_LOG(1) << "invalid visible region reference: " << visible_region_uri;
   return false;
}

bool Renderer::SetExploredRegion(std::string const& explored_region_uri)
{
   dm::Store const& store = Client::GetInstance().GetStore();
   om::Region2BoxedPtr exploredRegionBoxed = store.FetchObject<om::Region2Boxed>(explored_region_uri);

   if (exploredRegionBoxed) {
      exploredTrace_ = exploredRegionBoxed->TraceChanges("render explored region", dm::RENDER_TRACES)
                            ->OnModified([=](){
                               csg::Region2 exploredRegion = exploredRegionBoxed->Get();

                               Renderer::GetInstance().UpdateFoW(Renderer::GetInstance().fowExploredNode_, exploredRegion);
                               //int num_cubes = exploredRegion.GetCubeCount();
                               //R_LOG(3) << "Client explored cubes: " << num_cubes;
                            })
                            ->PushObjectState(); // Immediately send the current state to listener
      return true;
   }
   R_LOG(1) << "invalid explored region reference: " << explored_region_uri;
   return false;
}

void Renderer::RenderOneFrame(int now, float alpha)
{
   // Initialize all the new render entities we created this frame.
   platform::timer t(_maxRenderEntityLoadTime);

   while (!_newRenderEntities.empty() && !t.expired()) {
      std::shared_ptr<RenderEntity> re = _newRenderEntities.back().lock();
      _newRenderEntities.pop_back();
      if (re) {
         re->FinishConstruction();
      }
   }

   if (iconified_) {
      return;
   }

   perfmon::TimelineCounterGuard tcg("render one");

   bool debug = false;
   bool showStats = false;
   if (config_.enable_debug_keys.value) {
      debug = glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
      showStats = glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
   }
  
   bool showUI = true;
   const float ww = uiWidth_ / (float)uiHeight_;

   h3dSetOption(H3DOptions::DebugViewMode, debug);
   h3dSetOption(H3DOptions::WireframeMode, debug);
   // h3dSetOption(H3DOptions::DebugViewMode, _debugViewMode ? 1.0f : 0.0f);
   // h3dSetOption(H3DOptions::WireframeMode, _wireframeMode ? 1.0f : 0.0f);
   
   h3dSetCurrentRenderTime(now / 1000.0f);

   if (showUI && uiBuffer_.getMaterial()) { // show UI
      const float ovUI[] = { 0,  0, 0, 1, // flipped up-side-down!
                             0,  1, 0, 0,
                             ww, 1, 1, 0,
                             ww, 0, 1, 1, };
      h3dShowOverlays(ovUI, 4, 1, 1, 1, 1, uiBuffer_.getMaterial(), 0);
   }
   if (false) { // show color mat
      const float v[] = { ww * .9f, .9f,    0, 1, // flipped up-side-down!
                          ww * .9f,  1,     0, 0,
                          ww,        1,     1, 0,
                          ww,       .9f,    1, 1, };
   }

   perfmon::SwitchToCounter("render fire traces");
   int curWallTime = platform::get_current_time_in_ms();
   render_frame_start_slot_.Signal(FrameStartInfo(now, alpha, now - last_render_time_, curWallTime - last_render_time_wallclock_));

   if (showStats) { 
      perfmon::SwitchToCounter("show stats") ;  
      // show stats
      h3dCollectDebugFrame();
      h3dutShowFrameStats( fontMatRes_, panelMatRes_, H3DUTMaxStatMode );
   }

   h3dResetStats();

   perfmon::SwitchToCounter("render load res");
   fileWatcher_.update();
   LoadResources();

   if (camera_) {
      float skysphereDistance = config_.draw_distance.value * 0.4f;
      float starsphereDistance = config_.draw_distance.value * 0.9f;
      // Update the position of the sky so that it is always around the camera.  This isn't strictly
      // necessary for rendering, but very important for culling!  Horde doesn't (yet) know that
      // some nodes should _always_ be drawn.
      h3dSetNodeTransform(meshNode, 
         camera_->GetPosition().x, -(skysphereDistance * 0.5f) + camera_->GetPosition().y, camera_->GetPosition().z,
         25.0, 0.0, 0.0,
         skysphereDistance, skysphereDistance, skysphereDistance);
      h3dSetNodeTransform(starfieldMeshNode, 
         camera_->GetPosition().x, camera_->GetPosition().y, camera_->GetPosition().z,
         0.0, 0.0, 0.0, 
         starsphereDistance, starsphereDistance, starsphereDistance);

      // Render scene
      perfmon::SwitchToCounter("render h3d");
   
      if (drawWorld_) {
         currentPipeline_ = worldPipeline_;

         RenderFogOfWarRT();
         h3dSetResParamStr(GetPipeline(currentPipeline_), H3DPipeRes::GlobalRenderTarget, 0, fowRenderTarget_, "FogOfWarRT");
      }

      h3dRender(camera_->GetNode(), GetPipeline(currentPipeline_));

      // Finish rendering of frame
      UpdateCamera();
   }

   perfmon::SwitchToCounter("render finalize");
   h3dFinalizeFrame();

   // Advance emitter time; this must come AFTER rendering, because we only know which emitters
   // to update after doing a render pass.
   perfmon::SwitchToCounter("render ce");
   float delta = (now - last_render_time_) / 1000.0f;
   h3dRadiantAdvanceCubemitterTime(delta);
   h3dRadiantAdvanceAnimatedLightTime(delta);

   // Remove all overlays
   h3dClearOverlays();

   // Write all messages to log file
   h3dutDumpMessages();

   perfmon::SwitchToCounter("render swap");

   glfwSwapBuffers(glfwGetCurrentContext());

   last_render_time_ = now;
   last_render_time_wallclock_ = curWallTime;
}

bool Renderer::IsRunning() const
{
   return true;
}

void Renderer::GetCameraToViewportRay(int viewportX, int viewportY, csg::Ray3* ray)
{
   ASSERT(camera_);
   if (camera_) {
      // compute normalized window coordinates in preparation for casting a ray
      // through the scene
      float vw = (float)h3dGetNodeParamI(camera_->GetNode(), H3DCamera::ViewportWidthI);
      float vh = (float)h3dGetNodeParamI(camera_->GetNode(), H3DCamera::ViewportHeightI);

      float nwx = viewportX / (float)vw;
      float nwy = 1.0f - (viewportY / (float)vh);

      // calculate the ray starting at the eye position of the camera, casting
      // through the specified window coordinates into the scene
      h3dutPickRay(camera_->GetNode(), nwx, nwy,
                   &ray->origin.x, &ray->origin.y, &ray->origin.z,
                   &ray->direction.x, &ray->direction.y, &ray->direction.z);
   }
}

void Renderer::CastRay(const csg::Point3f& origin, const csg::Point3f& direction, int userFlags, RayCastHitCb cb)
{
   if (!rootRenderObject_) {
      return;
   }

   // Cast the ray from the root node to make sure we hit everything, including client-side created
   // entities which are not children of the game root entity.
   int num_results = h3dCastRay(1, origin.x, origin.y, origin.z,
                                direction.x, direction.y, direction.z, 10, userFlags);
   
   // Pull out the intersection node and intersection point
   for (int i = 0; i < num_results; i++) {
      H3DNode node;
      csg::Point3f intersection, normal;
      if (h3dGetCastRayResult(i, &node, 0, &intersection.x, &normal.x)) {
         normal.Normalize();
         cb(intersection, normal, node);
      }
   }
}

RaycastResult Renderer::QuerySceneRay(const csg::Point3f& origin, const csg::Point3f& direction, int userFlags)
{
   RaycastResult result(csg::Ray3(origin, direction));

   CastRay(origin, direction, userFlags, [this, &result](csg::Point3f const& intersection, csg::Point3f const& normal, H3DNode node) {

      // find the entity for the node that the ray hit.  walk up the hierarchy of nodes
      // until we find one that has an Entity associated with it.
      om::EntityRef entity;
      H3DNode n = node;
      while (n) {
         auto i = selectionLookup_.find(n);
         if (i != selectionLookup_.end()) {
            entity = i->second;
            break;
         }
         n = h3dGetNodeParent(n);
      }

      // Figure out the world voxel coordination of the intersection
      csg::Point3 brick;
      brick.x = csg::ToClosestInt(intersection.x);
      brick.z = csg::ToClosestInt(intersection.z);
      brick.y = csg::ToInt(intersection.y - (normal.y * 0.99f));
      result.AddResult(intersection, normal, brick, entity);
   });

   return result;
}

RaycastResult Renderer::QuerySceneRay(int viewportX, int viewportY, int userFlags)
{
   csg::Ray3 ray;
   GetCameraToViewportRay(viewportX, viewportY, &ray);
   return QuerySceneRay(ray.origin, ray.direction, userFlags);
}

csg::Matrix4 Renderer::GetNodeTransform(H3DNode node) const
{
   const float *abs;
   h3dGetNodeTransMats(node, NULL, &abs);

   csg::Matrix4 transform;
   transform.fill(abs);

   return transform;
 }

void Renderer::UpdateUITexture(const csg::Region2& rgn)
{
   if (!rgn.IsEmpty()) {
      uiBuffer_.update();
   }
}

void Renderer::HandleResize()
{
   if (resize_pending_)
   {
      resize_pending_ = false;
      OnWindowResized(nextWidth_, nextHeight_);
   }
}

void Renderer::ResizePipelines()
{
   for (const auto& entry : pipelines_) {
      h3dResizePipelineBuffers(entry.second, windowWidth_, windowHeight_);
   }
}

void Renderer::ResizeViewport()
{
   double desiredAspect = windowWidth_ / (double)windowHeight_;
   int left = 0;
   int top = 0;
   int width = windowWidth_;
   int height = windowHeight_;
   if (windowWidth_ < 1920 || windowHeight_ < 1080) {
      desiredAspect = 1920.0 / 1080.0;
      double aspect = windowWidth_ / (double)windowHeight_;

      double widthAspect = aspect > desiredAspect ? (aspect / desiredAspect) : 1.0;
      double widthResidual = (windowWidth_ - (windowWidth_ / widthAspect));
      double heightAspect = aspect < desiredAspect ? (desiredAspect / aspect) : 1.0;
      double heightResidual = (windowHeight_ - (windowHeight_ / heightAspect));
   
      left = (int)(widthResidual * 0.5);
      top = (int)(heightResidual * 0.5);
      width = (int)(windowWidth_ - widthResidual);
      height = (int)(windowHeight_ - heightResidual);
   }

   R_LOG(3) << "Window resized to " << windowWidth_ << "x" << windowHeight_ << ", viewport is (" << left << ", " << top << ", " << width << ", " << height << ")";

   // Resize viewport
   if (camera_) {
      H3DNode camera = camera_->GetNode();

      h3dSetOverlayAspectRatio((float)desiredAspect);
      h3dSetNodeParamI(camera, H3DCamera::ViewportXI, left);
      h3dSetNodeParamI(camera, H3DCamera::ViewportYI, top);
      h3dSetNodeParamI(camera, H3DCamera::ViewportWidthI, width);
      h3dSetNodeParamI(camera, H3DCamera::ViewportHeightI, height);
   
      // Set virtual camera parameters
      h3dSetupCameraView( camera, 45.0f, width / (float)height, 2.0f, config_.draw_distance.value);
   }
}

void Renderer::SetDrawWorld(bool drawWorld) 
{
   if (drawWorld_ == drawWorld) {
      return;
   }
   drawWorld_ = drawWorld;
   if (drawWorld_) {
      currentPipeline_ = worldPipeline_;
   } else {
      currentPipeline_ = "pipelines/ui_only.pipeline.xml";
   }
}

void* Renderer::GetNextUiBuffer()
{
   return uiBuffer_.getNextUiBuffer();
}

void* Renderer::GetLastUiBuffer()
{
   return uiBuffer_.getLastUiBuffer();
}


std::shared_ptr<RenderEntity> Renderer::CreateRenderEntity(H3DNode parent, om::EntityPtr entity)
{
   std::shared_ptr<RenderEntity> result = GetRenderEntity(entity);
   if (result) {
      result->SetParent(parent);
      return result;
   }

   dm::ObjectId id = entity->GetObjectId();
   int sid = entity->GetStoreId();

   R_LOG(5) << "creating render object " << sid << ", " << id;
   std::shared_ptr<RenderEntity> re = std::make_shared<RenderEntity>(parent, entity);
   _newRenderEntities.emplace_back(re);

   RenderMapEntry entry;
   entry.render_entity = re;
   entry.lifetime_trace = entity->TraceChanges("render dtor", dm::RENDER_TRACES)
                                    ->OnDestroyed([this, sid, id]() { 
                                          R_LOG(5) << "destroying render object in trace callback " << sid << ", " << id;
                                          entities_[sid].erase(id);
                                       });
   entities_[sid][id] = entry;
   return entry.render_entity;
}

std::shared_ptr<RenderEntity> Renderer::GetRenderEntity(om::EntityPtr entity)
{
   dm::ObjectId id = entity->GetObjectId();
   int sid = entity->GetStoreId();

   auto& entities = entities_[sid];
   auto i = entities.find(id);

   if (i != entities.end()) {
      RenderMapEntry const& entry = i->second;
      return entry.render_entity;
   }
   return nullptr;
}

void Renderer::UpdateCamera()
{
   ASSERT(camera_);

   // Let the audio listener know where the camera is
   csg::Point3f position = camera_->GetPosition();
   csg::Point3f direction = camera_->GetOrientation().rotate(csg::Point3f(0, 0, -1));
   sf::Listener::setPosition(position.x, position.y, position.z);
   sf::Listener::setDirection(direction.x, direction.y, direction.z);
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
   GetViewportMouseCoords(x, y);
   input_.mouse.dx = (int)x - input_.mouse.x;
   input_.mouse.dy = (int)y - input_.mouse.y;
   
   input_.mouse.x = (int)x;
   input_.mouse.y = (int)y;

   // xxx - this is annoying, but that's what you get... maybe revisit the
   // way we deliver mouse events and up/down tracking...
   memset(input_.mouse.up, 0, sizeof input_.mouse.up);
   memset(input_.mouse.down, 0, sizeof input_.mouse.down);

   if (input_.mouse.drag_start_x > 0 || input_.mouse.drag_start_y > 0) {
      if (std::abs(input_.mouse.drag_start_x - input_.mouse.x) > 1 || std::abs(input_.mouse.drag_start_y - input_.mouse.y) > 1) {
         input_.mouse.dragging = true;
      }
   } 
   
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

   if (press == GLFW_PRESS) {
      input_.mouse.drag_start_x = input_.mouse.x;
      input_.mouse.drag_start_y = input_.mouse.y;
   } else if (press == GLFW_RELEASE) {
      input_.mouse.drag_start_x = -1;
      input_.mouse.drag_start_y = -1;
   }

   input_.mouse.dragging = false;
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

void Renderer::GetViewportMouseCoords(double& x, double& y)
{
   if (camera_) {
      int vx = h3dGetNodeParamI(camera_->GetNode(), H3DCamera::ViewportXI);
      int vy = h3dGetNodeParamI(camera_->GetNode(), H3DCamera::ViewportYI);

      x = x - vx;
      y = y - vy;
   } else {
      x = y = 0;
   }
}

void Renderer::OnWindowResized(int newWidth, int newHeight) {
   if (newWidth == 0 || newHeight == 0) {
      return;
   }

   windowWidth_ = newWidth;
   windowHeight_ = newHeight;

   SetUITextureSize(windowWidth_, windowHeight_);
   ResizeViewport();
   ResizePipelines();
   CallWindowResizeListeners();
}

void Renderer::CallWindowResizeListeners() {
   if (camera_) {
      screen_resize_slot_.Signal(csg::Point2(
         h3dGetNodeParamI(camera_->GetNode(), H3DCamera::ViewportWidthI), 
         h3dGetNodeParamI(camera_->GetNode(), H3DCamera::ViewportHeightI)));
   }
}

void Renderer::OnFocusChanged(int wasFocused) {
   input_.focused = wasFocused == GL_FALSE ? false : true;

   DispatchInputEvent();
}

core::Guard Renderer::SetSelectionForNode(H3DNode node, om::EntityRef entity)
{
   selectionLookup_[node] = entity;
   return core::Guard([=]() {
      selectionLookup_.erase(node);
   });
}

H3DRes Renderer::GetPipeline(std::string const& name)
{
   H3DRes p = 0;

   auto i = pipelines_.find(name);
   if (i == pipelines_.end()) {
      p = h3dAddResource(H3DResTypes::Pipeline, name.c_str(), 0);
      pipelines_[name] = p;
      LoadResources();
      h3dResizePipelineBuffers(p, windowWidth_, windowHeight_);
   } else {
      p = i->second;
   }
   return p;
}

void Renderer::LoadResources()
{
   uiBuffer_.allocateBuffers(std::max(uiWidth_, 1920), std::max(1080, uiHeight_));

   if (!LoadMissingResources()) {
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

int Renderer::GetWindowWidth() const
{
   return windowWidth_;
}

int Renderer::GetWindowHeight() const
{
   return windowHeight_;
}

json::Node Renderer::GetTerrainConfig() const
{  
   return terrainConfig_;
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
   uiWidth_ = 1920;
   uiHeight_ = 1080;
   if (width >= 1920 && height >= 1080) {
      uiWidth_ = width;
      uiHeight_ = height;
   }

   uiBuffer_.allocateBuffers(uiWidth_, uiHeight_);
}

core::Guard Renderer::OnScreenResize(std::function<void(csg::Point2)> fn)
{
   return screen_resize_slot_.Register(fn);
}

core::Guard Renderer::OnServerTick(std::function<void(int)> fn)
{
   return server_tick_slot_.Register(fn);
}

core::Guard Renderer::OnRenderFrameStart(std::function<void(FrameStartInfo const&)> fn)
{
   return render_frame_start_slot_.Register(fn);
}

bool Renderer::ShowDebugShapes()
{
   return show_debug_shapes_;
}

void Renderer::SetShowDebugShapes(bool show_debug_shapes)
{
   show_debug_shapes_ = show_debug_shapes;
   show_debug_shapes_changed_slot_.Signal(show_debug_shapes);
}

core::Guard Renderer::OnShowDebugShapesChanged(std::function<void(bool)> fn)
{
   return show_debug_shapes_changed_slot_.Register(fn);
}

bool Renderer::LoadMissingResources()
{
   bool result = true;
   res::ResourceManager2& resourceManager = res::ResourceManager2::GetInstance();
   
   // Get the first resource that needs to be loaded
   int res = h3dQueryUnloadedResource(0);
   while( res != 0 ) {
      const char *resourceName = h3dGetResName(res);
      std::string resourcePath = resourcePath_ + "/" + resourceName;
      std::shared_ptr<std::istream> inf;

      // using exceptions here was a HORRIBLE idea.  who's responsible for this? =O - tony
      try {
         inf = resourceManager.OpenResource(resourcePath);
      } catch (std::exception const&) {
         R_LOG(0) << "failed to load render resource " << resourceName;
      }
      if (inf) {
         std::string buffer = io::read_contents(*inf);
         result = h3dLoadResource(res, buffer.c_str(), buffer.size()) && result;
      } else {
         // Tell engine to use the default resource by using NULL as data pointer
         h3dLoadResource(res, 0x0, 0);
         result = false;
      }
      // Get next unloaded resource
      res = h3dQueryUnloadedResource(0);
   }
   return result;
}
