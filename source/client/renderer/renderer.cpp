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

using namespace ::radiant;
using namespace ::radiant::client;

#define R_LOG(level)      LOG(renderer.renderer, level)

std::vector<float> ssaoSamplerData;

H3DRes ssaoMat;
H3DNode meshNode;
H3DNode starfieldMeshNode;
H3DRes starfieldMat;
H3DRes skysphereMat;

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
   camera_(nullptr),
   uiWidth_(0),
   uiHeight_(0),
   last_render_time_(0),
   server_tick_slot_("server tick"),
   render_frame_start_slot_("render frame start"),
   screen_resize_slot_("screen resize"),
   show_debug_shapes_changed_slot_("show debug shapes"),
   lastGlfwError_("none"),
   currentPipeline_(0),
   iconified_(false),
   resize_pending_(false),
   drawWorld_(false)
{
   terrainConfig_ = res::ResourceManager2::GetInstance().LookupJson("stonehearth/renderers/terrain/config.json");
   GetConfigOptions();

   assert(renderer_.get() == nullptr);
   renderer_.reset(this);

   glfwSetErrorCallback([](int errorCode, const char* errorString) {
      Renderer::GetInstance().lastGlfwError_ = BUILD_STRING(errorString << " (code: " << std::to_string(errorCode) << ")");
   });

   if (!glfwInit())
   {
      throw std::runtime_error(BUILD_STRING("Unable to initialize glfw: " << lastGlfwError_));
   }

   GLFWmonitor* monitor;
   int windowX, windowY;
   SelectSaneVideoMode(config_.enable_fullscreen.value, &windowWidth_, &windowHeight_, &windowX, &windowY, &monitor);
   config_.last_window_x.value = windowX;
   config_.last_window_y.value = windowY;
   config_.screen_height.value = windowHeight_;
   config_.screen_width.value = windowWidth_;

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

   if (!config_.enable_fullscreen.value) {
      SetWindowPos(GetWindowHandle(), NULL, windowX, windowY, 0, 0, SWP_NOSIZE);
   }
   // Init Horde, looking for OpenGL 2.0 minimum.
   std::string s = (radiant::core::System::GetInstance().GetTempDirectory() / "gfx.log").string();
   if (!h3dInit(2, 0, config_.enable_gl_logging.value, s.c_str())) {
      h3dutDumpMessages();
      throw std::runtime_error("Unable to initialize renderer.  Check horde log for details.");
   }

   // Set options
   h3dSetOption(H3DOptions::LoadTextures, 1);
   h3dSetOption(H3DOptions::TexCompression, 0);
   h3dSetOption(H3DOptions::MaxAnisotropy, 1);
   h3dSetOption(H3DOptions::FastAnimation, 1);
   h3dSetOption(H3DOptions::DumpFailedShaders, 1);

   ApplyConfig(config_, false);

   SetDrawWorld(drawWorld_);

   // Overlays
   fontMatRes_ = h3dAddResource( H3DResTypes::Material, "overlays/font.material.xml", 0 );
   panelMatRes_ = h3dAddResource( H3DResTypes::Material, "overlays/panel.material.xml", 0 );
   
   // xxx - should move this into the horde extension for debug shapes, but it doesn't know
   // how to actually get the resource loaded!
   h3dAddResource(H3DResTypes::Material, "materials/debug_shape.material.xml", 0); 
   
   ssaoMat = h3dAddResource(H3DResTypes::Material, "materials/ssao.material.xml", 0);

   csg::RandomNumberGenerator &rng = csg::RandomNumberGenerator::DefaultInstance();
   H3DRes veclookup = h3dCreateTexture("RandomVectorLookup", 4, 4, H3DFormats::TEX_RGBA32F, H3DResFlags::NoTexMipmaps);
   float *data2 = (float *)h3dMapResStream(veclookup, H3DTexRes::ImageElem, 0, H3DTexRes::ImgPixelStream, false, true);
   for (int i = 0; i < 16; i++)
   {
      float x = rng.GetReal(-1.0f, 1.0f);
      float y = rng.GetReal(-1.0f, 1.0f);
      float z = 0;
      Horde3D::Vec3f v(x,y,z);
      v.normalize();

      data2[(i * 4) + 0] = v.x;
      data2[(i * 4) + 1] = v.y;
      data2[(i * 4) + 2] = v.z;
   }
   h3dUnmapResStream(veclookup);

   BuildSkySphere();

   BuildStarfield();

   LoadResources();

   // Sampler kernel generation--a work in progress.
   const int KernelSize = 16;
   for (int i = 0; i < KernelSize; ++i) {
      float x = rng.GetReal(-1.0f, 1.0f);
      float y = rng.GetReal(-1.0f, 1.0f);
      float z = rng.GetReal(-0.5f, 0.5f);
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

   memset(&input_.mouse, 0, sizeof input_.mouse);
   input_.focused = true;

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
      RECT rect;
      GetWindowRect(Renderer::GetInstance().GetWindowHandle(), &rect);

      Renderer::GetInstance().config_.last_window_x.value = (int)rect.left;
      Renderer::GetInstance().config_.last_window_y.value = (int)rect.top;
      Renderer::GetInstance().config_.screen_width.value = Renderer::GetInstance().windowWidth_;
      Renderer::GetInstance().config_.screen_height.value = Renderer::GetInstance().windowHeight_;
      Renderer::GetInstance().ApplyConfig(Renderer::GetInstance().config_, true);

      R_LOG(0) << "window closed.  exiting process";
      TerminateProcess(GetCurrentProcess(), 1);
   });

   fileWatcher_.addWatch(strutil::utf8_to_unicode("horde"), [](FW::WatchID watchid, const std::wstring& dir, const std::wstring& filename, FW::Action action) -> void {
      Renderer::GetInstance().FlushMaterials();
   }, true);

   OnWindowResized(windowWidth_, windowHeight_);
   SetShowDebugShapes(false);

   initialized_ = true;
}

void Renderer::SetSkyColors(const csg::Point3f& startCol, const csg::Point3f& endCol)
{
   h3dSetMaterialUniform(skysphereMat, "skycolor_start", startCol.x, startCol.y, startCol.z, 1.0f);
   h3dSetMaterialUniform(skysphereMat, "skycolor_end", endCol.x, endCol.y, endCol.z, 1.0f);
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

   // "Uses the forward-renderer, instead of the deferred renderer."
   config_.use_forward_renderer.value = config.Get("renderer.use_forward_renderer", true);

   // "Enables SSAO blur."
   config_.use_ssao_blur.value = config.Get("renderer.use_ssao_blur", true);

   // "Enables shadows."
   config_.use_shadows.value = config.Get("renderer.enable_shadows", true);

   // "Enables Screen-Space Ambient Occlusion (SSAO)."
   config_.use_ssao.value = config.Get("renderer.enable_ssao", true);

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
}

void Renderer::ApplyConfig(const RendererConfig& newConfig, bool persistConfig)
{
   memcpy(&config_, &newConfig, sizeof(RendererConfig));

   H3DRendererCaps rendererCaps;
   H3DGpuCaps gpuCaps;
   h3dGetCapabilities(&rendererCaps, &gpuCaps);

   // Presently, only the engine can decide if certain features are even allowed to run.
   config_.num_msaa_samples.allowed = gpuCaps.MSAASupported;
   config_.use_shadows.allowed = rendererCaps.ShadowsSupported;

   if (config_.use_forward_renderer.value) {
      SetCurrentPipeline("pipelines/forward.pipeline.xml");
   } else {
      SetCurrentPipeline("pipelines/deferred_lighting.xml");
   }

   SetStageEnable("SSAO", config_.use_ssao.value);
   SetStageEnable("Simple, once-pass SSAO Blur", config_.use_ssao_blur.value);
   // Turn on copying if we're using SSAO, but not using blur.
   SetStageEnable("SSAO Copy", config_.use_ssao.value && !config_.use_ssao_blur.value);
   SetStageEnable("SSAO Default", !config_.use_ssao.value);

   int oldMSAACount = (int)h3dGetOption(H3DOptions::SampleCount);

   h3dSetOption(H3DOptions::EnableShadows, config_.use_shadows.value ? 1.0f : 0.0f);
   h3dSetOption(H3DOptions::ShadowMapSize, (float)config_.shadow_resolution.value);
   h3dSetOption(H3DOptions::SampleCount, (float)config_.num_msaa_samples.value);

   if (oldMSAACount != (int)h3dGetOption(H3DOptions::SampleCount))
   {
      // MSAA change requires that we reload our pipelines (so that we can regenerate our
      // render target textures with the appropriate sampling).
      FlushMaterials();

      LoadResources();
   }

   // Propagate far-plane value.
   if (camera_) {
      ResizeViewport();
   }

   glfwSwapInterval(config_.enable_vsync.value ? 1 : 0);

   if (persistConfig)
   {
      core::Config& config = core::Config::GetInstance();

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
   }

   // We just flushed/loaded our pipeline, so don't forget to reset the draw bits!
   SetDrawWorld(drawWorld_);
}

void Renderer::SelectSaneVideoMode(bool fullscreen, int* width, int* height, int* windowX, int* windowY, GLFWmonitor** monitor) 
{
   int lastX = config_.last_window_x.value;
   int lastY = config_.last_window_y.value;
   int numMonitors;

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
         break;
      }
   }
   if (desiredMonitor == NULL) {
      lastX = 0;
      lastY = 0;
      desiredMonitor = glfwGetPrimaryMonitor();
   }

   // At this point, we have a valid monitor, and a position.
   *monitor = desiredMonitor;
   *windowX = lastX;
   *windowY = lastY;

   int last_res_width = config_.screen_width.value;
   int last_res_height = config_.screen_height.value;
   const GLFWvidmode* m = glfwGetVideoMode(desiredMonitor);

   if (!fullscreen) {
      // If we're not fullscreen, then just ensure the size of the window <= the res of the monitor.
      *width = std::max(320, std::min(last_res_width, m->width));
      *height = std::max(240, std::min(last_res_height, m->height));
   } else {
      *width = m->width;
      *height = m->height;
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

void Renderer::SetStageEnable(const char* stageName, bool enabled)
{
   int stageCount = h3dGetResElemCount(currentPipeline_, H3DPipeRes::StageElem);

   for (int i = 0; i < stageCount; i++)
   {
      const char* curStageName = h3dGetResParamStr(currentPipeline_, H3DPipeRes::StageElem, i, H3DPipeRes::StageNameStr);
      if (_strcmpi(stageName, curStageName) == 0)
      {
         h3dSetResParamI(currentPipeline_, H3DPipeRes::StageElem, i, H3DPipeRes::StageActivationI, enabled ? 1 : 0);
         break;
      }
   }
}

void Renderer::FlushMaterials() {
   H3DRes r = 0;
   while ((r = h3dGetNextResource(H3DResTypes::Shader, r)) != 0) {
      h3dUnloadResource(r);
   }

   r = 0;
   while ((r = h3dGetNextResource(H3DResTypes::Material, r)) != 0) {
      if (!uiBuffer_.isUiMaterial(r))
      {
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

   ResizePipelines();
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

void Renderer::RenderOneFrame(int now, float alpha)
{
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

   perfmon::SwitchToCounter("render fire traces") ;  
   render_frame_start_slot_.Signal(FrameStartInfo(now, alpha, now - last_render_time_));

   if (showStats) { 
      perfmon::SwitchToCounter("show stats") ;  
      // show stats
      h3dCollectDebugFrame();
      h3dutShowFrameStats( fontMatRes_, panelMatRes_, H3DUTMaxStatMode );
   }

   perfmon::SwitchToCounter("render load res");
   fileWatcher_.update();
   LoadResources();

   h3dSetMaterialArrayUniform( ssaoMat, "samplerKernel", ssaoSamplerData.data(), ssaoSamplerData.size());
   
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
   h3dRender(camera_->GetNode());

   // Finish rendering of frame
   UpdateCamera();
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
}

bool Renderer::IsRunning() const
{
   return true;
}

void Renderer::GetCameraToViewportRay(int viewportX, int viewportY, csg::Ray3* ray)
{
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

void Renderer::CastScreenCameraRay(int viewportX, int viewportY, RayCastResult* result)
{
   result->is_valid = false;

   if (!rootRenderObject_) {
      return;
   }

   csg::Ray3 ray;
   GetCameraToViewportRay(viewportX, viewportY, &ray);

   CastRay(ray.origin, ray.direction, result);
}

void Renderer::QuerySceneRay(int viewportX, int viewportY, om::Selection &result)
{
   RayCastResult r;
   CastScreenCameraRay(viewportX, viewportY, &r);
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

void Renderer::ResizeWindow(int width, int height)
{
   windowWidth_ = width;
   windowHeight_ = height;

   SetUITextureSize(windowWidth_, windowHeight_);
}

void Renderer::ResizePipelines()
{
   for (const auto& entry : pipelines_) {
      h3dResizePipelineBuffers(entry.second, windowWidth_, windowHeight_);
   }
}

void Renderer::ResizeViewport()
{
   H3DNode camera = camera_->GetNode();

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
   h3dSetOverlayAspectRatio((float)desiredAspect);
   h3dSetNodeParamI( camera, H3DCamera::ViewportXI, left);
   h3dSetNodeParamI( camera, H3DCamera::ViewportYI, top);
   h3dSetNodeParamI( camera, H3DCamera::ViewportWidthI, width);
   h3dSetNodeParamI( camera, H3DCamera::ViewportHeightI, height);
   
   // Set virtual camera parameters
   h3dSetupCameraView( camera, 45.0f, width / (float)height, 2.0f, config_.draw_distance.value);
}

void Renderer::SetDrawWorld(bool drawWorld) 
{
   drawWorld_ = drawWorld;
   SetStageEnable("Sky", drawWorld);
   SetStageEnable("Starfield", drawWorld);
   SetStageEnable("Depth", drawWorld);
   SetStageEnable("Light", drawWorld);
   SetStageEnable("Clouds", drawWorld);
   SetStageEnable("Fog", drawWorld);
   SetStageEnable("Translucent", drawWorld);
   SetStageEnable("Selected", drawWorld);
}

void* Renderer::GetNextUiBuffer()
{
   return uiBuffer_.getNextUiBuffer();
}

void* Renderer::GetLastUiBuffer()
{
   return uiBuffer_.getLastUiBuffer();
}

std::shared_ptr<RenderEntity> Renderer::CreateRenderObject(H3DNode parent, om::EntityPtr entity)
{
   dm::ObjectId id = entity->GetObjectId();
   int sid = entity->GetStoreId();

   auto& entities = entities_[sid];
   auto i = entities.find(id);

   if (i != entities.end()) {
      RenderMapEntry const& entry = i->second;
      entry.render_entity->SetParent(parent);
      return entry.render_entity;
   }

   R_LOG(5) << "creating render object " << sid << ", " << id;
   RenderMapEntry entry;
   entry.render_entity = std::make_shared<RenderEntity>(parent, entity);
   entry.render_entity->FinishConstruction();
   entry.lifetime_trace = entity->TraceChanges("render dtor", dm::RENDER_TRACES)
                                    ->OnDestroyed([this, sid, id]() { 
                                          R_LOG(5) << "destroying render object in trace callback " << sid << ", " << id;
                                          entities_[sid].erase(id);
                                       });
   entities[id] = entry;
   return entry.render_entity;
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
      return i->second.render_entity;
   }
   return nullptr;
}

void Renderer::RemoveRenderObject(int sid, dm::ObjectId id)
{
   auto i = entities_[sid].find(id);
   if (i != entities_[sid].end()) {
      R_LOG(5) << "destroying render object (sid:" << sid << " id:" << id << ")";
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
   GetViewportMouseCoords(x, y);
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

void Renderer::GetViewportMouseCoords(double& x, double& y)
{
   int vx = h3dGetNodeParamI(camera_->GetNode(), H3DCamera::ViewportXI);
   int vy = h3dGetNodeParamI(camera_->GetNode(), H3DCamera::ViewportYI);

   x = x - vx;
   y = y - vy;
}

void Renderer::OnWindowResized(int newWidth, int newHeight) {
   if (newWidth == 0 || newHeight == 0)
   {
      return;
   }

   ResizeWindow(newWidth, newHeight);
   ResizeViewport();
   ResizePipelines();
   screen_resize_slot_.Signal(csg::Point2(
      h3dGetNodeParamI(camera_->GetNode(), H3DCamera::ViewportWidthI), 
      h3dGetNodeParamI(camera_->GetNode(), H3DCamera::ViewportHeightI)));
}

void Renderer::OnFocusChanged(int wasFocused) {
   input_.focused = wasFocused == GL_FALSE ? false : true;

   DispatchInputEvent();
}

core::Guard Renderer::TraceSelected(H3DNode node, UpdateSelectionFn fn)
{
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

void Renderer::LoadResources()
{
   uiBuffer_.allocateBuffers(std::max(uiWidth_, 1920), std::max(1080, uiHeight_));
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
   return windowWidth_;
}

int Renderer::GetHeight() const
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
   if (width > 1920 && height > 1080) {
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
