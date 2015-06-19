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
#include "resources/res_manager.h"
#include "csg/iterators.h"
#include "csg/random_number_generator.h"
#include "pipeline.h"
#include <unordered_set>
#include <string>
#include "client/client.h"
#include "raycast_result.h"
#include "platform/utils.h"
#include "horde3d\Source\Shared\utMath.h"
#include "gfxcard_db.h"

#ifdef WIN32
#include <Shellapi.h>
#endif

using namespace ::radiant;
using namespace ::radiant::client;

#define R_LOG(level)      LOG(renderer.renderer, level)

H3DNode meshNode;
H3DNode starfieldMeshNode;
H3DRes starfieldMat;
H3DRes skysphereMat;
H3DNode cursorNode;

#define MAX_FOW_NODES 10000

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
   screenSize_(csg::Point2::zero),
   uiSize_(csg::Point2::zero),
   last_render_time_(0),
   server_tick_slot_("server tick"),
   render_frame_start_slot_("render frame start"),
   render_frame_finished_slot_("render frame finished"),
   screen_resize_slot_("screen resize"),
   show_debug_shapes_changed_slot_("show debug shapes", 0),
   lastGlfwError_("none"),
   currentPipeline_(""),
   iconified_(false),
   resize_pending_(false),
   drawWorld_(true),
   last_render_time_wallclock_(0),
   _loading(false),
   _loadingAmount(0),
   screenshotTexRes_(0),
   portraitTexRes_(0)
{
   OneTimeIninitializtion();
}

std::string Renderer::GetGraphicsCardName() const
{
   std::string name;

#ifdef WIN32
   DISPLAY_DEVICE devInfo;
   devInfo.cb = sizeof(DISPLAY_DEVICE);

   DWORD iDevNum = 0;

   while (EnumDisplayDevices(NULL, iDevNum, &devInfo, 0))
   {
      // Access DevInfo here to get information on the current device...
      iDevNum++;
      if (devInfo.StateFlags & (DISPLAY_DEVICE_ATTACHED_TO_DESKTOP | DISPLAY_DEVICE_PRIMARY_DEVICE)) {
         name = std::string(devInfo.DeviceString);
         break;
      }
   }
#endif

   return name;
}

void Renderer::OneTimeIninitializtion()
{
   GetConfigOptions();
   
   if (!config_.run_once.value) {
      std::string gfxCard = GetGraphicsCardName();
      SelectRecommendedGfxLevel(gfxCard);
	  MaskHighQualitySettings();
   }
   config_.run_once.value = true;

   assert(renderer_.get() == nullptr);
   renderer_.reset(this);

   csg::Point2 windowSize = InitWindow();
   InitHorde();
   SetupGlfwHandlers();
   MakeRendererResources();

   ApplyConfig(config_, APPLY_CONFIG_RENDERER);
   LoadResources();

   memset(&input_.mouse, 0, sizeof input_.mouse);
   input_.focused = true;

   // If the mod is unzipped, put a watch on the filesystem directory where the resources live
   // so we can dynamically load resources whenever the file changes.
   if (core::Config::GetInstance().Get<bool>("enable_renderer_file_watcher", false)) {
      std::string fspath = std::string("mods/");
      if (boost::filesystem::is_directory(fspath)) {
         fileWatcher_.addWatch(strutil::utf8_to_unicode(fspath), [](FW::WatchID watchid, const std::wstring& dir, const std::wstring& filename, FW::Action action) -> void {
            const std::wstring sufx[] = {L".shader", L".json", L".png", L".tga", L".jpg", L".glsl", L".xml", L".state"};

            for (int i = 0; i < 8; i++) {
               if (filename.length() - sufx[i].length() - 1 < 0) {
                  continue;
               }
               if (filename.compare(filename.length() - sufx[i].length() - 1, sufx[i].length(), sufx[i]) == 0) {
                  Renderer::GetInstance().FlushMaterials();
               }
            }
         }, true);
      }
   }

   OnWindowResized(windowSize);
   SetShowDebugShapes(0);

   SetDrawWorld(false);
   initialized_ = true;
}


void Renderer::SelectRecommendedGfxLevel(std::string const& gfxCard)
{
   int gpuScore = GetGpuPerformance(gfxCard);

   // Slightly aggressive: low-quality renderer, but at 1080P and some light shadowing.
   if (gpuScore == 0) {
      gpuScore = 750;
   }

   config_.enable_vsync.value = false;
   if (gpuScore <= 500) {
      config_.use_high_quality.value = false;

      config_.screen_width.value = 1280;
      config_.screen_height.value = 720;
      config_.draw_distance.value = 500;
      config_.use_shadows.value = false;
   } else if (gpuScore <= 1000) {
      config_.use_high_quality.value = false;

      config_.screen_width.value = 1280;
      config_.screen_height.value = 720;
      config_.draw_distance.value = 1000;
      config_.use_shadows.value = true;
      config_.shadow_quality.value = 1;
   } else if (gpuScore <= 1500) {
      config_.use_high_quality.value = false;

      config_.screen_width.value = 1920;
      config_.screen_height.value = 1080;
      config_.draw_distance.value = 1000;
      config_.use_shadows.value = true;
      config_.shadow_quality.value = 2;
   } else if (gpuScore <= 2500) {
      config_.use_high_quality.value = true;

      config_.screen_width.value = 1920;
      config_.screen_height.value = 1080;
      config_.draw_distance.value = 1000;
      config_.use_shadows.value = true;
      config_.enable_ssao.value = false;
      config_.shadow_quality.value = 2;
      config_.num_msaa_samples.value = 0;
   } else if (gpuScore <= 3500) {
      config_.use_high_quality.value = true;

      config_.screen_width.value = 1920;
      config_.screen_height.value = 1080;
      config_.draw_distance.value = 1000;
      config_.use_shadows.value = true;
      config_.enable_ssao.value = false;
      config_.shadow_quality.value = 3;
      config_.num_msaa_samples.value = 1;
   } else if (gpuScore <= 4500) {
      config_.use_high_quality.value = true;

      config_.screen_width.value = 1920;
      config_.screen_height.value = 1080;
      config_.draw_distance.value = 1000;
      config_.use_shadows.value = true;
      config_.shadow_quality.value = 4;
      config_.enable_ssao.value = true;
      config_.num_msaa_samples.value = 1;
   } else {
      config_.use_high_quality.value = true;

      config_.screen_width.value = 1920;
      config_.screen_height.value = 1080;
      config_.draw_distance.value = 1000;
      config_.use_shadows.value = true;
      config_.shadow_quality.value = 4;
      config_.enable_ssao.value = true;
      config_.num_msaa_samples.value = 1;
   }
   // anti-aliasing, god rays, etc., to come.
}

int Renderer::GetGpuPerformance(std::string const& gfxCard) const
{
   JSONNode rootj;
   std::string error_message;
   json::ReadJson(std::string(gfxCardData), rootj, error_message);
   json::Node root(rootj);

   std::vector<std::string> tokens;
   std::stringstream ss(gfxCard);
   std::string item;

   // Split the gpu name ('NVIDIA GeForce 900 GTX') into tokens, used to traverse our gpu trie.
   while (std::getline(ss, item, ' ')) {
      tokens.push_back(item);
   }

   // First, we traverse the gpu trie to find the deepest matching node.
   json::Node n = root;
   while (tokens.size() > 0) {
      int idx = -1;
      for (uint i = 0; i < tokens.size(); i++) {
         if (n.has(tokens[i])) {
            n = n.get_node(tokens[i]);
            idx = i;
            break;
         }
      }

      if (idx == -1) {
         tokens.clear();
      } else {
         tokens.erase(tokens.begin() + idx);
      }
   }

   // We might be at a non-leaf node, but on branch with exactly one descendant leaf.  If so,
   // traverse to that leaf.  (We're returning what appears to be the 'best-matching' gpu, in this 
   // case.)
   while (!n.has("_value") && n.size() == 1) {
      n = n.get_node(0);
   }

   // If this is a leaf, then return the value; otherwise, return 0 ('unknown').
   return n.get("_value", 0);
}


void Renderer::MakeRendererResources()
{
   // Overlays
   fontMatRes_ = h3dAddResource( H3DResTypes::Material, "overlays/font.material.json", 0 );
   panelMatRes_ = h3dAddResource( H3DResTypes::Material, "overlays/panel.material.json", 0 );

   H3DRendererCaps caps;
   h3dGetCapabilities(&caps, nullptr);

   if (caps.HighQualityRendererSupported) {
      H3DRes veclookup = h3dCreateTexture("RandomVectorLookup", 4, 4, H3DFormats::TEX_RGBA32F, H3DResFlags::NoTexMipmaps | H3DResFlags::NoQuery | H3DResFlags::NoFlush);

      csg::RandomNumberGenerator &rng = csg::RandomNumberGenerator::DefaultInstance();
      float *data2 = (float *)h3dMapResStream(veclookup, H3DTexRes::ImageElem, 0, H3DTexRes::ImgPixelStream, false, true);

      if (data2) {
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
         h3dUnmapResStream(veclookup, 0);
      }

      H3DRes sampleLookup = h3dCreateTexture("SampleVectorLookup", 1, 16, H3DFormats::TEX_RGBA32F, H3DResFlags::NoTexMipmaps | H3DResFlags::NoQuery | H3DResFlags::NoFlush);

      data2 = (float *)h3dMapResStream(veclookup, H3DTexRes::ImageElem, 0, H3DTexRes::ImgPixelStream, false, true);

      if (data2) {
         data2[0] = 0.120735723f;
         data2[1] = 0.154695181f;
         data2[2] = 0.095559036f;
         data2[3] = 0.0;

         data2[4] = -0.17934270f;
         data2[5] = -0.133400726f;
         data2[6] = 0.180391080f;
         data2[7] = 0.0;

         data2[8] = -0.183279236f;
         data2[9] = -0.260888325f;
         data2[10] = 0.179007574f;
         data2[11] = 0.0;

         data2[12] = -0.209957821f;
         data2[13] = 0.303312696f;
         data2[14] = 0.263813869f;
         data2[15] = 0.0;

         data2[16] = 0.250082046f;
         data2[17] = 0.32981852f;
         data2[18] = 0.363463802f;
         data2[19] = 0.0;

         data2[20] = 0.4226088961f;
         data2[21] = -0.058862660f;
         data2[22] = 0.500400324f;
         data2[23] = 0.0;

         data2[24] = 0.3573904081f;
         data2[25] = -0.473313856f;
         data2[26] = 0.497081545f;
         data2[27] = 0.0;

         data2[28] = 0.5012853241f;
         data2[29] = -0.470811146f;
         data2[30] = 0.579836830f;
         data2[31] = 0.0;

         h3dUnmapResStream(sampleLookup, 0);
      }

      CreateTextureResource("SMAA_AreaTex", resourcePath_ + "/textures/smaa/smaa_area.raw", 160, 560, H3DFormats::TEX_RG8, 2);
      CreateTextureResource("SMAA_SearchTex", resourcePath_ + "/textures/smaa/smaa_search.raw", 66, 33, H3DFormats::TEX_R8, 1);
   }


   fowRenderTarget_ = h3dutCreateRenderTarget(512, 512, H3DFormats::TEX_BGRA8, false, 1, 0, 0);

   BuildLoadingScreen();
}

void Renderer::InitHorde()
{
   // Init Horde, looking for OpenGL 2.0 minimum.
   if (!h3dInit(2, 0, false, config_.enable_gl_logging.value)) {
      throw std::runtime_error("Unable to initialize renderer.  Check horde log for details.");
   }

   // Set options
   h3dSetOption(H3DOptions::LoadTextures, 1);
   h3dSetOption(H3DOptions::TexCompression, 0);
   h3dSetOption(H3DOptions::MaxAnisotropy, 1);
   h3dSetOption(H3DOptions::FastAnimation, 1);
   h3dSetOption(H3DOptions::DumpFailedShaders, 1);
   h3dSetOption(H3DOptions::SRGBLinearization, 0);

   h3dSetGlobalShaderFlag("DRAW_GRIDLINES", false);
}

void DumpDisplayAdapters() {
#ifdef WIN32
   DISPLAY_DEVICE dd;
   dd.cb = sizeof(dd);
   int num = 0;
   while (EnumDisplayDevices(nullptr, num, &dd, 1)) {
      R_LOG(1) << " Display device " << num << ": " << dd.DeviceString;
      num++;
   }
#endif
}

void ShowDriverUpdateWindow() {
#ifdef WIN32
   int id = MessageBox(NULL, "Press 'okay' to go to stonehearth.net to learn how to upgrade your graphics card drivers.", "Could not find OpenGL.",  MB_OKCANCEL);
   if (id == IDOK) {
      ShellExecute(NULL, "open", "http://stonehearth.net/some-technical-stuff/", NULL, NULL, SW_SHOWMAXIMIZED);
   }
#endif
}


csg::Point2 Renderer::InitWindow()
{
   glfwSetErrorCallback([](int errorCode, const char* errorString) {
      if (errorCode != 0) {
         R_LOG(1) << "GLFW Error (" << errorCode << "): " << errorString;
         Renderer::GetInstance().lastGlfwError_ = BUILD_STRING(errorString << " (code: " << std::to_string(errorCode) << ")");
         
         if (errorCode == GLFW_API_UNAVAILABLE) {
            ShowDriverUpdateWindow();
         }
      } else {
         // Error code 0 is used to 
         R_LOG(1) << "glfw: " << errorString;
      }
   });

   R_LOG(1) << "Initializing OpenGL";
   if (!glfwInit())
   {
      throw std::runtime_error(BUILD_STRING("Unable to initialize glfw: " << lastGlfwError_));
   }

   R_LOG(1) << "Determining window placement";
   inFullscreen_ = config_.enable_fullscreen.value;
   GLFWmonitor* monitor;
   csg::Point2 size, pos;
   SelectSaneVideoMode(config_.enable_fullscreen.value, pos, size, &monitor);

   if (!inFullscreen_) {
      config_.last_window_x.value = pos.x;
      config_.last_window_y.value = pos.y;
      config_.screen_width.value = size.x;
      config_.screen_height.value = size.y;
   }

   glfwWindowHint(GLFW_SRGB_CAPABLE, 0);
   glfwWindowHint(GLFW_SAMPLES, 0);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
   glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, config_.enable_gl_logging.value ? 1 : 0);

   R_LOG(1) << "Creating OpenGL Window";
   GLFWwindow *window = glfwCreateWindow(size.x, size.y, "Stonehearth", 
                                         config_.enable_fullscreen.value ? monitor : nullptr, nullptr);
   if (!window) {
      R_LOG(1) << "Error trying to create glfw window.  (size:" << size << "  fullscreen:" << config_.enable_fullscreen.value << ")";
      glfwTerminate();
      DumpDisplayAdapters();
      throw std::runtime_error(BUILD_STRING("Unable to create glfw window: " << lastGlfwError_));
   }

   R_LOG(1) << "Creating OpenGL Context";
   glfwMakeContextCurrent(window);
   glfwGetWindowSize(window, &size.x, &size.y);

   if (!inFullscreen_) {
      SetWindowPos(GetWindowHandle(), NULL, pos.x, pos.y, 0, 0, SWP_NOSIZE);
   }

   if (config_.minimized.value) {
      glfwIconifyWindow(window);
   }
   R_LOG(1) << "Finished OpenGL Initialization";
   return size;
}

void Renderer::SetupGlfwHandlers()
{
   GLFWwindow *window = glfwGetCurrentContext();
   glfwSetWindowSizeCallback(window, [](GLFWwindow *window, int newWidth, int newHeight) {       
      Renderer &r = Renderer::GetInstance();
      r.resize_pending_ = true;
      r.nextScreenSize_ = csg::Point2(newWidth, newHeight);
   });

   glfwSetWindowIconifyCallback(window, [](GLFWwindow *window, int iconified) {
      Renderer::GetInstance().iconified_ = iconified == GL_TRUE;
   });

   glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scancode, int action, int mods) { 
      if (action == GLFW_REPEAT) {
         return;
      }
      Renderer::GetInstance().OnKey(key, action == GLFW_PRESS, mods); 
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

   glfwSetScrollCallback(window, [](GLFWwindow *window, double dx, double dy) {
      Renderer::GetInstance().OnMouseWheel(dy);
   });

   // Platform-specific ifdef code goes here....
   glfwSetRawInputCallbackWin32(window, [](GLFWwindow *window, UINT uMsg, WPARAM wParam, LPARAM lParam) {
      Renderer::GetInstance().OnRawInput(uMsg, wParam, lParam);
   });
   
   glfwSetWindowCloseCallback(window, [](GLFWwindow* window) -> void {
      Renderer::GetInstance().OnWindowClose();
   });
}

void Renderer::UpdateFoW(H3DNode node, const csg::Region2f& region)
{
   if (region.GetCubeCount() >= MAX_FOW_NODES) {
      R_LOG(1) << "Reached limit on fow nodes!  Is region optimization broken?";
      return;
   }
   float* start = (float*)h3dMapNodeParamV(node, H3DInstanceNodeParams::InstanceBuffer);
   float* f = start;
   float ySize = 100.0f;
   for (const auto& c : csg::EachCube(region)) 
   {
      float px = static_cast<float>((c.max + c.min).x) * 0.5f;
      float pz = static_cast<float>((c.max + c.min).y) * 0.5f;

      float xSize = (float)(c.max.x - c.min.x);
      float zSize = (float)(c.max.y - c.min.y);
      f[0] = xSize; f[1] = 0;                f[2] =   0;    f[3] =  0;
      f[4] = 0;     f[5] = ySize;            f[6] =   0;    f[7] =  0;
      f[8] = 0;     f[9] = 0;                f[10] = zSize; f[11] = 0;
      f[12] = px;   f[13] = -ySize/2.0f;     f[14] =  pz;   f[15] = 1;
      
      f += 16;
   }

   h3dUnmapNodeParamV(node, H3DInstanceNodeParams::InstanceBuffer, (int)(f - start) * 4);
   csg::Rect2f bounds = region.GetBounds();
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

   float cascadeBound = std::floor((camFrust.getCorner(0) - camFrust.getCorner(6)).length());
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
   h3dSetMaterialUniform(skysphereMat, "skycolor_start", (float)startCol.x, (float)startCol.y, (float)startCol.z, 1.0f);
   h3dSetMaterialUniform(skysphereMat, "skycolor_end", (float)endCol.x, (float)endCol.y, (float)endCol.z, 1.0f);
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
      2, 0, 3,   3, 0, 4,   4, 0, 5,   5, 0, 2,   2, 3, 1,   3, 4, 1,   4, 5, 1,   5, 2, 1
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

   skysphereMat = h3dAddResource(H3DResTypes::Material, "materials/skysphere.material.json", 0);
   H3DRes geoRes = h3dutCreateGeometryRes("skysphere", 2046, 2048 * 3, (float*)spVerts, spInds, nullptr, nullptr, nullptr, texData, nullptr);
   H3DNode modelNode = h3dAddModelNode(mainSceneRoot_, "skysphere_model", geoRes);
   meshNode = h3dAddMeshNode(modelNode, "skysphere_mesh", skysphereMat, 0, 2048 * 3, 0, 2045);
   h3dSetNodeFlags(modelNode, H3DNodeFlags::NoCastShadow | H3DNodeFlags::NoRayQuery | H3DNodeFlags::NoCull, true);
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

   starfieldMat = h3dAddResource(H3DResTypes::Material, "materials/starfield.material.json", 0);

   H3DRes geoRes = h3dutCreateGeometryRes("starfield", NumStars * 4, NumStars * 6, (float*)verts, indices, nullptr, nullptr, nullptr, texCoords, texCoords2);
   
   H3DNode modelNode = h3dAddModelNode(mainSceneRoot_, "starfield_model", geoRes);
   starfieldMeshNode = h3dAddMeshNode(modelNode, "starfield_mesh", starfieldMat, 0, NumStars * 6, 0, NumStars * 4 - 1);
   h3dSetNodeFlags(modelNode, H3DNodeFlags::NoCull | H3DNodeFlags::NoCastShadow | H3DNodeFlags::NoRayQuery, true);
}

// These options should be worded so that they can default to false
void Renderer::GetConfigOptions()
{
   const core::Config& config = core::Config::GetInstance();

   config_.use_high_quality.value = config.Get("renderer.use_high_quality", false);

   config_.enable_ssao.value = config.Get("renderer.enable_ssao", false);

   // "Enables shadows."
   config_.use_shadows.value = config.Get("renderer.enable_shadows", true);

   // "Sets the number of Multi-Sample Anti Aliasing samples to use."
   config_.num_msaa_samples.value = config.Get("renderer.msaa_samples", 0);

   // "Sets the square resolution of the shadow maps."
   config_.shadow_quality.value = config.Get("renderer.shadow_quality", 3);

   config_.max_lights.value = config.Get("renderer.max_lights", 50);

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

   config_.minimized.value = config.Get("renderer.minimized", false);

   config_.disable_pinned_memory.value = config.Get("renderer.disable_pinned_memory", false);

   config_.run_once.value = config.Get("renderer.run_once", false);

   config_.dump_compiled_shaders.value = config.Get("renderer.dump_compiled_shaders", false);
   
   _maxRenderEntityLoadTime = core::Config::GetInstance().Get<int>("max_render_entity_load_time", 50);

   resourcePath_ = config.Get("renderer.resource_path", "stonehearth/data/horde");

   minUiSize_.x = config.Get("renderer.min_ui_width",  1920);
   minUiSize_.y = config.Get("renderer.min_ui_height", 1080);

   MaskHighQualitySettings();
}

void Renderer::MaskHighQualitySettings() 
{
   bool high_quality = config_.use_high_quality.value;
   // Mask all high-quality settings with the 'use_high_quality' bool.
   config_.enable_ssao.value &= high_quality;
   config_.num_msaa_samples.value &= high_quality ? 0xFF : 0x00;
}

void Renderer::UpdateConfig(const RendererConfig& newConfig)
{
   memcpy(&config_, &newConfig, sizeof(RendererConfig));

   H3DRendererCaps rendererCaps;
   H3DGpuCaps gpuCaps;
   h3dGetCapabilities(&rendererCaps, &gpuCaps);

   // Presently, only the engine can decide if certain features are even allowed to run.
   config_.use_shadows.allowed = rendererCaps.ShadowsSupported;
   config_.enable_ssao.allowed = rendererCaps.HighQualityRendererSupported;

   config_.use_high_quality.allowed = rendererCaps.HighQualityRendererSupported;

   config_.use_high_quality.value &= config_.use_high_quality.allowed;

   MaskHighQualitySettings();
}

void Renderer::PersistConfig()
{
   core::Config& config = core::Config::GetInstance();

   config.Set("renderer.run_once", config_.run_once.value);

   config.Set("renderer.use_high_quality", config_.use_high_quality.value);

   config.Set("renderer.enable_ssao", config_.enable_ssao.value);

   config.Set("renderer.enable_shadows", config_.use_shadows.value);
   config.Set("renderer.msaa_samples", config_.num_msaa_samples.value);

   // It's possible the engine will override the user's settings; so, listen to the engine.
   config.Set("renderer.shadow_quality", (int)h3dGetOption(H3DOptions::ShadowMapQuality));

   config.Set("renderer.max_lights", config_.max_lights.value);

   config.Set("renderer.enable_vsync", config_.enable_vsync.value);

   config.Set("renderer.enable_fullscreen", config_.enable_fullscreen.value);

   config.Set("renderer.screen_width", config_.screen_width.value);
   config.Set("renderer.screen_height", config_.screen_height.value);
   config.Set("renderer.draw_distance", config_.draw_distance.value);

   config.Set("renderer.last_window_x", config_.last_window_x.value);
   config.Set("renderer.last_window_y", config_.last_window_y.value);

   config.Set("renderer.last_screen_x", config_.last_screen_x.value);
   config.Set("renderer.last_screen_y", config_.last_screen_y.value);
}

void Renderer::ApplyConfig(const RendererConfig& newConfig, int flags)
{
   UpdateConfig(newConfig);

   if ((flags & APPLY_CONFIG_RENDERER) != 0) {
      // Super hard-coded setting for now.
      if (config_.use_high_quality.value) {
         worldPipeline_ = "pipelines/forward_postprocess.pipeline.xml";
      } else {
         worldPipeline_ = "pipelines/forward.pipeline.xml";
      }

      h3dSetOption(H3DOptions::EnableShadows, config_.use_shadows.value ? 1.0f : 0.0f);
      h3dSetOption(H3DOptions::ShadowMapQuality, (float)config_.shadow_quality.value);
      h3dSetOption(H3DOptions::MaxLights, (float)config_.max_lights.value);
      h3dSetOption(H3DOptions::DisablePinnedMemory, config_.disable_pinned_memory.value);
      h3dSetOption(H3DOptions::DumpCompiledShaders, config_.dump_compiled_shaders.value);


      SelectPipeline();

      // Set up the optional stages for the high-quality pipeline.
      if (config_.use_high_quality.value) {
         SetStageEnable(GetPipeline(currentPipeline_), "Collect SSAO", config_.enable_ssao.value);
         SetStageEnable(GetPipeline(currentPipeline_), "SSAO Contribution", config_.enable_ssao.value);

         SetStageEnable(GetPipeline(currentPipeline_), "FSAA", config_.num_msaa_samples.value > 0);
         SetStageEnable(GetPipeline(currentPipeline_), "Composite", config_.num_msaa_samples.value == 0);
      }

      // Propagate far-plane value.
      ResizeViewport();

      glfwSwapInterval(config_.enable_vsync.value ? 1 : 0);
   }

   if ((flags & APPLY_CONFIG_PERSIST) != 0) {
      PersistConfig();
   }
}

GLFWmonitor* getMonitorAt(int x, int y)
{
   int numMonitors;
   GLFWmonitor** monitors = glfwGetMonitors(&numMonitors);
   for (int i = 0; i < numMonitors; i++) 
   {
      int monitorX, monitorY;
      glfwGetMonitorPos(monitors[i], &monitorX, &monitorY);
      const GLFWvidmode* m = glfwGetVideoMode(monitors[i]);
      
      if ((x >= monitorX && y >= monitorY) && 
         (x < monitorX + m->width && y < monitorY + m->height)) 
      {
         return monitors[i];
      }
   }

   return nullptr;
}

void Renderer::SelectSaneVideoMode(bool fullscreen, csg::Point2 &pos, csg::Point2 &size, GLFWmonitor** monitor) 
{
   pos.x = config_.last_window_x.value;
   pos.y = config_.last_window_y.value;

   int last_res_width = config_.screen_width.value;
   int last_res_height = config_.screen_height.value;

   if (!fullscreen) {
      // If we're not fullscreen, then just ensure the size of the window <= the res of the monitor.
      R_LOG(1) << "Selecting monitor at " << config_.last_screen_x.value << ", " << config_.last_screen_y.value;
      *monitor = getMonitorAt(config_.last_screen_x.value, config_.last_screen_y.value);

      if (*monitor == NULL) {
         // Couldn't find a monitor to contain the window; put us on the first monitor.
         R_LOG(1) << "Could not find monitor.  Using primary.";
         *monitor = glfwGetPrimaryMonitor();
         ASSERT(*monitor);
         pos.x = 0;
         pos.y = 0;
      }
      const GLFWvidmode* m = glfwGetVideoMode(*monitor);
      size.x = std::max(320, std::min(last_res_width, m->width));
      size.y = std::max(240, std::min(last_res_height, m->height));
   } else {
      // In fullscreen, try to find the monitor that contains the window's upper-left coordinate.
      GLFWmonitor *desiredMonitor = getMonitorAt(config_.last_screen_x.value, config_.last_screen_y.value);
      R_LOG(1) << "Selecting fullscreen monitor at " << config_.last_screen_x.value << ", " << config_.last_screen_y.value;
      if (desiredMonitor == NULL) {
         R_LOG(1) << "Could not find monitor.  Using primary.";
         desiredMonitor = glfwGetPrimaryMonitor();
         ASSERT(desiredMonitor);
      }
      const GLFWvidmode* m = glfwGetVideoMode(desiredMonitor);
      *monitor = desiredMonitor;
      size.x = m->width;
      size.y = m->height;
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
      if (!(h3dGetResFlags(r) & H3DResFlags::NoFlush) && h3dIsResLoaded(r)) {
         h3dUnloadResource(r);
      }
   }

   r = 0;
   while ((r = h3dGetNextResource(H3DResTypes::ShaderState, r)) != 0) {
      if (!(h3dGetResFlags(r) & H3DResFlags::NoFlush) && h3dIsResLoaded(r)) {
         h3dUnloadResource(r);
      }
   }

   r = 0;
   while ((r = h3dGetNextResource(H3DResTypes::Material, r)) != 0) {
      if (!(h3dGetResFlags(r) & H3DResFlags::NoFlush) && h3dIsResLoaded(r)) {
         h3dUnloadResource(r);
      }
   }

   r = 0;
   while ((r = h3dGetNextResource(H3DResTypes::Pipeline, r)) != 0) {
      if (!(h3dGetResFlags(r) & H3DResFlags::NoFlush) && h3dIsResLoaded(r)) {
         h3dUnloadResource(r);
      }
   }

   r = 0;
   while ((r = h3dGetNextResource(H3DResTypes::ParticleEffect, r)) != 0) {
      if (!(h3dGetResFlags(r) & H3DResFlags::NoFlush) && h3dIsResLoaded(r)) {
         h3dUnloadResource(r);
      }
   }

   r = 0;
   while ((r = h3dGetNextResource(H3DResTypes::Code, r)) != 0) {
      if (!(h3dGetResFlags(r) & H3DResFlags::NoFlush) && h3dIsResLoaded(r)) {
         h3dUnloadResource(r);
      }
   }

   r = 0;
   while ((r = h3dGetNextResource(RT_CubemitterResource, r)) != 0) {
      if (!(h3dGetResFlags(r) & H3DResFlags::NoFlush) && h3dIsResLoaded(r)) {
         h3dUnloadResource(r);
      }
   }

   r = 0;
   while ((r = h3dGetNextResource(RT_AnimatedLightResource, r)) != 0) {
      if (!(h3dGetResFlags(r) & H3DResFlags::NoFlush) && h3dIsResLoaded(r)) {
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
   rootRenderObject_ = CreateRenderEntity(mainSceneRoot_, rootObject);
}

void Renderer::Initialize()
{
   mainSceneRoot_ = h3dGetRootNode(0);
   fowSceneRoot_ = h3dGetRootNode(h3dAddScene("fow"));
   portraitSceneRoot_ = h3dGetRootNode(h3dAddScene("portrait"));

   RenderNode::Initialize();

   BuildSkySphere();
   BuildStarfield();

   csg::Region3::Cube littleCube(csg::Region3::Point(0, 0, 0), csg::Region3::Point(1, 1, 1));
   /*fowVisibleNode_ = h3dAddInstanceNode(fowSceneRoot_, "fow_visiblenode", 
      h3dAddResource(H3DResTypes::Material, "materials/fow_visible.material.json", 0), 
      Pipeline::GetInstance().CreateVoxelGeometryFromRegion("littlecube", littleCube), 1000);*/
   fowExploredNode_ = h3dAddInstanceNode(fowSceneRoot_, "fow_explorednode", 
      h3dAddResource(H3DResTypes::Material, "materials/fow_explored.material.json", 0), 
      Pipeline::GetInstance().CreateVoxelGeometryFromRegion("littlecube", littleCube), MAX_FOW_NODES);
   h3dSetNodeFlags(fowExploredNode_, H3DNodeFlags::NoCastShadow | H3DNodeFlags::NoRayQuery | H3DNodeFlags::NoCull, true);

   csg::Region2f r;
   r.Add(csg::Rect2f(csg::Point2f(-1000, -1000), csg::Point2f(1000, 1000)));
   UpdateFoW(fowExploredNode_, r);

   // Add camera   
   camera_ = new Camera(mainSceneRoot_, "Camera");

   // Add another camera--this is exclusively for the fog-of-war pipeline.
   fowCamera_ = new Camera(fowSceneRoot_, "FowCamera");

   portraitCamera_ = new Camera(portraitSceneRoot_, "PortraitCamera");

   int portraitWidth = 128, portraitHeight = 128;

   if (portraitTexRes_ == 0) {
      portraitTexRes_ = h3dCreateTexture("portraitTexture", portraitWidth, portraitHeight, H3DFormats::TEX_BGRA8, H3DResFlags::NoTexMipmaps | H3DResFlags::NoQuery | H3DResFlags::NoFlush | H3DResFlags::TexRenderable);
   }
   h3dSetNodeParamI(portraitCamera_->GetNode(), H3DCamera::ViewportXI, 0);
   h3dSetNodeParamI(portraitCamera_->GetNode(), H3DCamera::ViewportYI, 0);
   h3dSetNodeParamI(portraitCamera_->GetNode(), H3DCamera::ViewportWidthI, portraitWidth);
   h3dSetNodeParamI(portraitCamera_->GetNode(), H3DCamera::ViewportHeightI, portraitHeight);
   h3dSetNodeParamI(portraitCamera_->GetNode(), H3DCamera::OutTexResI, portraitTexRes_);
   h3dSetupCameraView(portraitCamera_->GetNode(), 45.0f, 1.0, 2.0f, 500.0f);

   debugShapes_ = h3dRadiantAddDebugShapes(mainSceneRoot_, "renderer debug shapes");

   // We now have a camera, so update our viewport, and inform all interested parties of
   // the update.
   ResizeViewport();
   CallWindowResizeListeners();

   portrait_requested_ = false;
   portrait_generated_ = false;

   last_render_time_ = 0;
   last_render_time_wallclock_ = platform::get_current_time_in_ms();
}

void Renderer::BuildLoadingScreen()
{
   _loadingBackgroundMaterial = h3dAddResource(H3DResTypes::Material, "materials/loading_screen.material.json", H3DResFlags::NoFlush);
   // Can't clone until we load!
   LoadResources();
   _loadingProgressMaterial = h3dCloneResource(_loadingBackgroundMaterial, "progress_material");

   // That those who built Horde thought *this* was a good idea is...remarkable....
   int numSamplers = h3dGetResElemCount(_loadingProgressMaterial, H3DMatRes::SamplerElem);
   for (int i = 0; i < numSamplers; i++) {
      std::string samplerName(h3dGetResParamStr(_loadingProgressMaterial, H3DMatRes::SamplerElem, i, H3DMatRes::SampNameStr));
      if (samplerName == "progressMap") {
         h3dSetResParamI(_loadingProgressMaterial, H3DMatRes::SamplerElem, i, H3DMatRes::SampTexResI, h3dAddResource(H3DResTypes::Texture, "overlays/meter_stretch.png", 0));
         break;
      }
   }
}

void Renderer::Shutdown()
{
   RenderNode::Shutdown();

   show_debug_shapes_changed_slot_.Clear();
   server_tick_slot_.Clear();
   render_frame_start_slot_.Clear();
   render_frame_finished_slot_.Clear();

   exploredTrace_ = nullptr;
   visibilityTrace_ = nullptr;
   rootRenderObject_ = nullptr;

   // Two pass destruction of our entity stores: destroy the data-structure containing them,
   // and then destroy the entities themselves.  This is necessary because destroying an entity
   // can, through recursive destruction of its children, cause lookups against the entities_
   // data structure, which must be in a valid state for the lookup.  So, in this case, we'll just
   // empty the data structure so that all lookups fail when the entities themselves are destroyed.
   {
      RenderEntityMap keepAlive[NUM_STORES];
      for (int i = 0; i < NUM_STORES; i++) {
         keepAlive[i] = entities_[i];
         entities_[i].clear();
      }
   }
   debugShapes_ = 0;
   fowExploredNode_ = 0;

   delete portraitCamera_; portraitCamera_ = nullptr;
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
   om::Region2fBoxedPtr visibleRegionBoxed = store.FetchObject<om::Region2fBoxed>(visible_region_uri);

   if (visibleRegionBoxed) {
      visibilityTrace_ = visibleRegionBoxed->TraceChanges("render visible region", dm::RENDER_TRACES)
                            ->OnModified([=](){
                               csg::Region2f visibleRegion = visibleRegionBoxed->Get();
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
   om::Region2fBoxedPtr exploredRegionBoxed = store.FetchObject<om::Region2fBoxed>(explored_region_uri);

   if (exploredRegionBoxed) {
      exploredTrace_ = exploredRegionBoxed->TraceChanges("render explored region", dm::RENDER_TRACES)
                            ->OnModified([=](){
                               csg::Region2f exploredRegion = exploredRegionBoxed->Get();

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

void Renderer::ConstructAllRenderEntities()
{
   while (!_newRenderEntities.empty()) {
      std::vector<std::weak_ptr<RenderEntity>> renderEntities = _newRenderEntities;
      _newRenderEntities.clear();
      for (std::weak_ptr<RenderEntity> re : renderEntities) {
         std::shared_ptr<RenderEntity> r = re.lock();
         if (r) {
            r->FinishConstruction();
         }
      }
   }
}

int Renderer::GetLastRenderTime() const
{
   return last_render_time_;
}

float Renderer::GetLastRenderAlpha() const
{
   return last_render_alpha_;
}

void Renderer::RequestPortrait(PortraitRequestCb const& fn)
{
   portrait_requested_ = true;
   portrait_generated_ = false;
   portrait_cb_ = fn;
}

void Renderer::RenderPortraitRT()
{
   perfmon::TimelineCounterGuard tcg("render portrait");

   // Render and return the image from the portrait scene.  We do this in two stages to give the renderer the chance
   // to actually get some work done before blocking on the texture read.
   if (portrait_requested_) {
      portrait_requested_ = false;
      portrait_generated_ = true;

      // Turn off the UI to render the portrait.
      SetStageEnable(GetPipeline(worldPipeline_), "Overlays", false);
      SetStageEnable(GetPipeline(worldPipeline_), "PortraitClear", true);
      h3dSetOption(H3DOptions::EnableRenderCaching, false);
      h3dRender(portraitCamera_->GetNode(), GetPipeline(worldPipeline_));
      SetStageEnable(GetPipeline(worldPipeline_), "PortraitClear", false);
      h3dSetOption(H3DOptions::EnableRenderCaching, true);
   } else if (portrait_generated_) {
      portrait_generated_ = false;
      perfmon::SwitchToCounter("encode portrait png");
      h3dutCreatePngImageFromTexture(portraitTexRes_, portraitBytes_);

      {
         perfmon::TimelineCounterGuard tcg("render portrait callback");
         portrait_cb_(portraitBytes_);
      }

      portraitBytes_.clear();
   }
}

void Renderer::RenderOneFrame(int now, float alpha, bool screenshot)
{
   perfmon::TimelineCounterGuard tcg("render one");

   ASSERT(now >= last_render_time_);

   // Initialize all the new render entities we created this frame.
   platform::timer t(_maxRenderEntityLoadTime);

   {
      perfmon::TimelineCounterGuard tcg("create render entities");
      
      while (!_newRenderEntities.empty() && !t.expired()) {
         std::shared_ptr<RenderEntity> re = _newRenderEntities.back().lock();
         _newRenderEntities.pop_back();
         if (re) {
            re->FinishConstruction();
         }
      }
   }

   if (iconified_) {
      return;
   }


   bool debug = false;
   bool showStats = false;
   if (config_.enable_debug_keys.value) {
      debug = glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
      showStats = glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
   }
  
   bool showUI = true;
   const float ww = uiSize_.x / (float)uiSize_.y;

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
   FrameStartInfo frameInfo(now, alpha, now - last_render_time_, curWallTime - last_render_time_wallclock_);
   render_frame_start_slot_.Signal(frameInfo);

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

   RenderPortraitRT();

   if (camera_) {
      float skysphereDistance = config_.draw_distance.value * 0.4f;
      float starsphereDistance = config_.draw_distance.value * 0.9f;
      // Update the position of the sky so that it is always around the camera.  This isn't strictly
      // necessary for rendering, but very important for culling!  Horde doesn't (yet) know that
      // some nodes should _always_ be drawn.
      csg::Point3f cp = camera_->GetPosition();
      float cx = (float)cp.x, cy = (float)cp.y, cz = (float)cp.z;

      h3dSetNodeTransform(meshNode, 
         cx, -(skysphereDistance * 0.5f) + cy, cz,
         25.0, 0.0, 0.0,
         skysphereDistance, skysphereDistance, skysphereDistance);

      h3dSetNodeTransform(starfieldMeshNode, 
         cx, cy, cz,
         0.0, 0.0, 0.0, 
         starsphereDistance, starsphereDistance, starsphereDistance);

      // Render scene
      perfmon::SwitchToCounter("render h3d");
   
      if (drawWorld_ && !_loading) {
         RenderFogOfWarRT();
         h3dSetResParamStr(GetPipeline(currentPipeline_), H3DPipeRes::GlobalRenderTarget, 0, fowRenderTarget_, "FogOfWarRT");
      }

      if (_loading) {
         RenderLoadingMeter();
      }

      SetStageEnable(GetPipeline(currentPipeline_), "Overlays", !screenshot);

      if (screenshot) {
         h3dSetNodeParamI(camera_->GetNode(), H3DCamera::OutTexResI, screenshotTexRes_);
      } else {
         h3dSetNodeParamI(camera_->GetNode(), H3DCamera::OutTexResI, 0);
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
   h3dAdvanceAnimatedTextureTime(delta);

   // Remove all overlays
   h3dClearOverlays();

   perfmon::SwitchToCounter("render swap");

   if (!screenshot) {
      glfwSwapBuffers(glfwGetCurrentContext());
   }

   h3dReleaseUnusedResources();

   render_frame_finished_slot_.Signal(frameInfo);

   last_render_time_ = now;
   last_render_alpha_ = alpha;
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
      float ox, oy, oz, dx, dy, dz;
      h3dutPickRay(camera_->GetNode(), nwx, nwy,
                   &ox, &oy, &oz, &dx, &dy, &dz);

      ray->origin.x = ox;
      ray->origin.y = oy;
      ray->origin.z = oz;
      ray->direction.x = dx;
      ray->direction.y = dy;
      ray->direction.z = dz;
   }
}

void Renderer::CastRay(const csg::Point3f& origin, const csg::Point3f& direction, int userFlags, RayCastHitCb const& cb)
{
   if (!rootRenderObject_) {
      return;
   }

   // Cast the ray from the root node to make sure we hit everything, including client-side created
   // entities which are not children of the game root entity.
   int num_results = h3dCastRay(mainSceneRoot_, (float)origin.x, (float)origin.y, (float)origin.z,
                                (float)direction.x, (float)direction.y, (float)direction.z, 10, userFlags);
   H3DSceneId scene = h3dGetSceneForNode(mainSceneRoot_);
   
   // Pull out the intersection node and intersection point
   for (int i = 0; i < num_results; i++) {
      H3DNode node;
      float intersection[3], normal[3];

      if (h3dGetCastRayResult(scene, i, &node, 0, intersection, normal)) {
         cb(csg::Point3f(intersection[0], intersection[1], intersection[2]),
            csg::Point3f(normal[0], normal[1], normal[2]).Normalized(),
            node);
      }
   }
}

RaycastResult Renderer::QuerySceneRay(const csg::Point3f& origin, const csg::Point3f& direction, int userFlags)
{
   RaycastResult result(csg::Ray3(origin, direction));

   CastRay(origin, direction, userFlags, [this, &result](csg::Point3f const& intersection, csg::Point3f const& normal, H3DNode node) {
      // get the name of the node we hit
      const char *node_name = h3dGetNodeParamStr(node, H3DNodeParams::NameStr);

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

      // Figure out the world voxel coordination of the intersection.
      // Recall that terrain aligned voxels go from -0.5 to +0.5 of the xz coordinate
      // but go from 0 to 1 of the y coordiante.
      csg::Point3 brick;
      // For (x,z), side face intersections are a face with a half-integer (0.5) coordinate
      // so, nudge inside the voxel and round to the center.
      // For top/bottom intersections, normal.x and normal.z will be zero.
      brick.x = csg::ToClosestInt(intersection.x - (normal.x * 0.1));
      brick.z = csg::ToClosestInt(intersection.z - (normal.z * 0.1));
      // For y, top/bottom intersections are a whole integer coordinate + potential polygon offset
      // so, nudge inside the voxel, and floor to the bottom face which defines the voxel coordinate.
      // These nudge coefficients are quite large to overcome any polygon offsets. This is ok as long
      // as they are < 0.5.
      // For side intersections, normal.y will be zero.
      brick.y = (int)std::floor(intersection.y - (normal.y * 0.1));
      result.AddResult(intersection, normal, brick, entity, node_name);
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

void Renderer::UpdateUITexture(csg::Point2 const& size, csg::Region2 const & rgn, const radiant::uint32* buff)
{
   uiSize_ = size;
   uiBuffer_.update(size, rgn, buff);
}

void Renderer::HandleResize()
{
   if (resize_pending_) {
      resize_pending_ = false;
      OnWindowResized(nextScreenSize_);
   }
}

void Renderer::ResizePipelines()
{
   for (const auto& entry : pipelines_) {
      h3dResizePipelineBuffers(entry.second, screenSize_.x, screenSize_.y);
   }
}

void Renderer::ResizeViewport()
{
   if (screenSize_ == csg::Point2::zero) {
      return;
   }

   double desiredAspect = screenSize_.x / (double)screenSize_.y;
   int left = 0;
   int top = 0;
   int width = screenSize_.x;
   int height = screenSize_.y;
   if (screenSize_.x < minUiSize_.x || screenSize_.y < minUiSize_.y) {
      desiredAspect = minUiSize_.x / (double)minUiSize_.y;
      double aspect = screenSize_.x / (double)screenSize_.y;

      double widthAspect = aspect > desiredAspect ? (aspect / desiredAspect) : 1.0;
      double widthResidual = (screenSize_.x - (screenSize_.x / widthAspect));
      double heightAspect = aspect < desiredAspect ? (desiredAspect / aspect) : 1.0;
      double heightResidual = (screenSize_.y - (screenSize_.y / heightAspect));
   
      left = (int)(widthResidual * 0.5);
      top = (int)(heightResidual * 0.5);
      width = (int)(screenSize_.x - widthResidual);
      height = (int)(screenSize_.y - heightResidual);
   }

   R_LOG(3) << "Window resized to " << screenSize_ << ", viewport is (" << left << ", " << top << ", " << width << ", " << height << ")";


   if (screenshotTexRes_) {
      h3dRemoveResource(screenshotTexRes_);
      h3dReleaseUnusedResources();
   }
   screenshotTexRes_ = h3dCreateTexture("screenshotTexture", screenSize_.x, screenSize_.y, H3DFormats::TEX_BGRA8, H3DResFlags::NoTexMipmaps | H3DResFlags::NoQuery | H3DResFlags::NoFlush | H3DResFlags::TexRenderable);

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
   drawWorld_ = drawWorld;
   SelectPipeline();
}

void Renderer::SetLoading(bool loading)
{
   _loading = loading;
   if (loading) {
      _loadingAmount = 0.0f;
   }
   SelectPipeline();
}

void Renderer::SelectPipeline()
{
   if (_loading) {
      currentPipeline_ = "pipelines/loading_screen_only.pipeline.xml";
      return;
   }
   if (drawWorld_) {
      currentPipeline_ = worldPipeline_;
      return;
   }
   currentPipeline_ = "pipelines/ui_only.pipeline.xml";
}


void Renderer::UpdateLoadingProgress(float amountLoaded)
{
   _loadingAmount = amountLoaded;
}

void Renderer::RenderLoadingMeter()
{
   // So...basically...don't ever mess with these values.  They are all magic, hand-crafted,
   // *artisnal* constants.
   // If you look closely, you'll see that the 'x' values don't even make sense--that's
   // because 'y' is [0,1] and 'x' is [0, Aspect_Ratio].  Awesome!
   float x = 0.62f;
   float y = 0.6f;
   float barHeight = 0.1f;
   float width = 0.5f;
   float ovTitleVerts[] = { x, y, 0, 0, x, y + barHeight, 0, 1,
	                           x + width, y + barHeight, 1, 1, x + width, y, 1, 0 };
   h3dShowOverlays( ovTitleVerts, 4,  1.0f, 1.0f, 1.0f, 1.0f, _loadingBackgroundMaterial, 0 );


   x = 0.64f;
   y = 0.613f;
   barHeight = 0.052f;
   width = 0.46f;
   ovTitleVerts[0] = x;
   ovTitleVerts[1] = y;
   ovTitleVerts[4] = x;
   ovTitleVerts[5] = y + barHeight;
   ovTitleVerts[8] = x + (width * _loadingAmount); 
   ovTitleVerts[9] = y + barHeight;
   ovTitleVerts[12] = x + (width * _loadingAmount);
   ovTitleVerts[13] = y;
         
   h3dShowOverlays( ovTitleVerts, 4,  1.0f, 1.0f, 1.0f, 1.0f, _loadingProgressMaterial, 0 );
}

std::shared_ptr<RenderEntity> Renderer::CreateRenderEntity(H3DNode parent, om::EntityPtr entity)
{
   std::shared_ptr<RenderEntity> result = GetRenderEntity(entity);
   if (result) {
      if (parent != RenderNode::GetUnparentedNode()) {
         result->SetParent(parent);
      }
      return result;
   }
   dm::ObjectId id = entity->GetObjectId();
   int sid = entity->GetStoreId();

   RenderMapEntry entry;
   entry.render_entity = CreateUnmanagedRenderEntity(parent, entity);
   entry.lifetime_trace = entity->TraceChanges("render dtor", dm::RENDER_TRACES)
                                    ->OnDestroyed([this, sid, id]() { 
                                          R_LOG(5) << "destroying render object in trace callback " << sid << ", " << id;
                                          entities_[sid].erase(id);
                                       });
   entities_[sid][id] = entry;
   return entry.render_entity;
}


std::shared_ptr<RenderEntity> Renderer::CreateUnmanagedRenderEntity(H3DNode parent, om::EntityPtr entity)
{
   dm::ObjectId id = entity->GetObjectId();
   int sid = entity->GetStoreId();

   R_LOG(5) << "creating render object " << sid << ", " << id;
   std::shared_ptr<RenderEntity> re = std::make_shared<RenderEntity>(parent, entity);
   _newRenderEntities.emplace_back(re);

   return re;
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
   sf::Listener::setPosition((float)position.x, (float)position.y, (float)position.z);
   sf::Listener::setDirection((float)direction.x, (float)direction.y, (float)direction.z);
}

void Renderer::OnMouseWheel(double dy)
{
   input_.mouse.wheel = (int)dy;
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
      if (std::abs(input_.mouse.drag_start_x - input_.mouse.x) > 10 || std::abs(input_.mouse.drag_start_y - input_.mouse.y) > 10) {
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

   // Update dragging state _after_ calling back.  Otherwise, clients will see one
   // frame with no dragging and a key-release, instead of a frame with a key release,
   // followed by a frame with no dragging.
   if (press == GLFW_PRESS) {
      input_.mouse.drag_start_x = input_.mouse.x;
      input_.mouse.drag_start_y = input_.mouse.y;
   } else if (press == GLFW_RELEASE) {
      input_.mouse.drag_start_x = -1;
      input_.mouse.drag_start_y = -1;
      input_.mouse.dragging = false;
   }

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

void Renderer::OnKey(int key, int down, int mods)
{
   bool handled = false, uninstall = false;

   input_.type = Input::KEYBOARD;
   input_.keyboard.down = down != 0;
   input_.keyboard.key = key;
   input_.keyboard.shift = (mods & GLFW_MOD_SHIFT) != 0;
   input_.keyboard.ctrl = (mods & GLFW_MOD_CONTROL) != 0;
   input_.keyboard.alt = (mods & GLFW_MOD_ALT) != 0;

   auto isKeyDown = [](WPARAM arg) {
      int keycode = (int)(arg);
      return (keycode & 0x8000) != 0;
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

void Renderer::OnWindowResized(csg::Point2 const& size) {
   if (size == screenSize_) {
      R_LOG(1) << "ignoring duplicate screen resize to " << size;
      return;
   }
   if (size == csg::Point2::zero) {
      R_LOG(1) << "ignoring screen resize to " << size;
      return;
   }
   screenSize_ = size;

   ResizeViewport();
   ResizePipelines();
   CallWindowResizeListeners();
}

void Renderer::CallWindowResizeListeners() {
   if (camera_) {
      H3DNode cameraNode = camera_->GetNode();
      csg::Point2 min(h3dGetNodeParamI(cameraNode, H3DCamera::ViewportXI),
                      h3dGetNodeParamI(cameraNode, H3DCamera::ViewportYI));
      csg::Point2 max(h3dGetNodeParamI(cameraNode, H3DCamera::ViewportWidthI), 
				          h3dGetNodeParamI(cameraNode, H3DCamera::ViewportHeightI));
      max += min;

      screen_resize_slot_.Signal(csg::Rect2(min, max));
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

H3DRes Renderer::GetScreenshotTexture() const
{
   return screenshotTexRes_;
}

H3DRes Renderer::GetPipeline(std::string const& name)
{
   H3DRes p = 0;

   auto i = pipelines_.find(name);
   if (i == pipelines_.end()) {
      p = h3dAddResource(H3DResTypes::Pipeline, name.c_str(), 0);
      pipelines_[name] = p;
      LoadResources();
      h3dResizePipelineBuffers(p, screenSize_.x, screenSize_.y);
   } else {
      p = i->second;
   }
   return p;
}

void Renderer::CreateTextureResource(std::string const& name, std::string const& path, int width, int height, int format, int stride)
{
   H3DRes texRes = h3dCreateTexture(name.c_str(), width, height, format, H3DResFlags::NoTexMipmaps | H3DResFlags::NoQuery | H3DResFlags::NoFlush);

   res::ResourceManager2& resourceManager = res::ResourceManager2::GetInstance();
   std::shared_ptr<std::istream> inf = resourceManager.OpenResource(path);
   std::string buffer = io::read_contents(*inf);
   unsigned char* data = (unsigned char*)h3dMapResStream(texRes, H3DTexRes::ImageElem, 0, H3DTexRes::ImgPixelStream, false, true);
   memcpy(data, buffer.c_str(), width * height * stride);
   h3dUnmapResStream(texRes, 0);
}

void Renderer::LoadResources()
{
   if (!LoadMissingResources()) {
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

csg::Point2 const& Renderer::GetScreenSize() const
{
   return screenSize_;
}

csg::Point2 const& Renderer::GetMinUiSize() const
{
   return minUiSize_;
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

core::Guard Renderer::OnScreenResize(std::function<void(csg::Rect2)> const& fn)
{
   return screen_resize_slot_.Register(fn);
}

core::Guard Renderer::OnServerTick(std::function<void(int)> const& fn)
{
   return server_tick_slot_.Register(fn);
}

core::Guard Renderer::OnRenderFrameStart(std::function<void(FrameStartInfo const&)> const& fn)
{
   return render_frame_start_slot_.Register(fn);
}

core::Guard Renderer::OnRenderFrameFinished(std::function<void(FrameStartInfo const&)> const& fn)
{
   return render_frame_finished_slot_.Register(fn);
}

void Renderer::SetShowDebugShapes(dm::ObjectId id)
{
   show_debug_shapes_changed_slot_.Signal(id);
}

core::Guard Renderer::OnShowDebugShapesChanged(std::function<void(dm::ObjectId)> const& fn)
{
     return show_debug_shapes_changed_slot_.Register(fn);
}

bool Renderer::LoadMissingResources()
{
   bool result = true;
   res::ResourceManager2& resourceManager = res::ResourceManager2::GetInstance();
   
   // Get the first resource that needs to be loaded
   int res = h3dQueryUnloadedResource(0);
   while (res != 0) {
      const char *resourceName = h3dGetResName(res);
      std::string rname(resourceName);
      std::string resourcePath;

      // Paths beginning with '/' are treated as relative to whatever mod from which they originate.
      if (rname[0] == '/') {
         resourcePath = rname.substr(1);
      } else {    
         // No leading '/' means look in the main stonehearth mod for the resource.
         resourcePath = resourcePath_ + "/" + resourceName;
      }
      std::shared_ptr<std::istream> inf;

      // using exceptions here was a HORRIBLE idea.  who's responsible for this? =O - tony
      try {
         inf = resourceManager.OpenResource(resourcePath);
      } catch (std::exception const&) {
         R_LOG(0) << "failed to load render resource " << resourceName;
      }
      if (inf) {
         std::string buffer = io::read_contents(*inf);
         result = h3dLoadResource(res, buffer.c_str(), (int)buffer.size()) && result;
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

void Renderer::OnWindowClose()
{
   // All Win32 specific!
   HWND hwnd = GetWindowHandle();

   // This nonsense is because the upper-left corner of the window (on a multi-monitor system)
   // will actually bleed into the adjacent window, so we need to figure out what kind of bias
   // that needs to be _in_ the monito  We get that bias by calling ScreenToClient on the
   // upper-left of the window's rect.
   RECT rect;
   GetWindowRect(hwnd, &rect);

   POINT p  = { 0 };
   POINT p2 = { rect.left, rect.top };

   ScreenToClient(hwnd, &p2);
   ClientToScreen(hwnd, &p);

   if (!inFullscreen_) {
      config_.last_window_x.value = (int)rect.left;
      config_.last_window_y.value = (int)rect.top;

      config_.last_screen_x.value = p.x - p2.x;
      config_.last_screen_y.value = p.y - p2.y;

      config_.screen_width.value = screenSize_.x;
      config_.screen_height.value = screenSize_.y;
   }
   ApplyConfig(config_, APPLY_CONFIG_PERSIST);

   R_LOG(0) << "window closed.  exiting process";
   TerminateProcess(GetCurrentProcess(), 1);
}
