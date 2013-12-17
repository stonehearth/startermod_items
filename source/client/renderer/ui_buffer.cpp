#include "pch.h"
#include "ui_buffer.h"
#include "radiant.h"
#include "radiant_stdutil.h"
#include "Horde3DUtils.h"
#include "Horde3DRadiant.h"
#include "lib/perfmon/perfmon.h"

using namespace ::radiant;
using namespace ::radiant::client;

#define R_LOG(level)      LOG(renderer.renderer, level)

UiBuffer::UiBuffer() :
   uiTexture_(0),
   uiMatRes_(0),
   uiPbo_(0),
   width_(0),
   height_(0)
{

}

UiBuffer::~UiBuffer()
{

}

H3DRes UiBuffer::getMaterial() const 
{
   return uiMatRes_;
}

void UiBuffer::buffersWereCleared()
{
   uiPbo_ = 0x0;
   uiMatRes_ = 0x0;

   allocateBuffers(width_, height_);
}

void UiBuffer::update(const char* buffer)
{
   if (!uiPbo_) {
      return;
   }
   perfmon::SwitchToCounter("map ui pbo");

   char *data = (char *)h3dMapResStream(uiPbo_, 0, 0, 0, false, true);

   if (data) {
      perfmon::SwitchToCounter("copy client mem to ui pbo");
      // If you think this is slow (bliting everything instead of just the dirty rects), please
      // talk to Klochek; the explanation is too large to fit in the margins of this code....
      memcpy(data, buffer, width_ * height_ * 4);
   }
   perfmon::SwitchToCounter("unmap ui pbo");
   h3dUnmapResStream(uiPbo_);

   perfmon::SwitchToCounter("copy ui pbo to ui texture") ;

   h3dCopyBufferToBuffer(uiPbo_, uiTexture_, 0, 0, width_, height_);
}

void UiBuffer::allocateBuffers(int width, int height)
{
   if (width != width_ || height != height_) {
      width_ = width;
      height_ = height;
   } else {
      return;
   }

   if (uiPbo_) {
      h3dRemoveResource(uiPbo_);
      uiPbo_ = 0x0;
   }
   if (uiTexture_) {
      h3dRemoveResource(uiTexture_);
      uiTexture_ = 0x0;
   }
   if (uiMatRes_) {
      h3dUnloadResource(uiMatRes_);
      uiMatRes_ = 0x0;

   }
   h3dReleaseUnusedResources();

   uiPbo_ = h3dCreatePixelBuffer("screenui", width * height * 4);

   uiTexture_ = h3dCreateTexture("UI Texture", width, height, H3DFormats::List::TEX_BGRA8, H3DResFlags::NoTexMipmaps);
   unsigned char *data = (unsigned char *)h3dMapResStream(uiTexture_, H3DTexRes::ImageElem, 0, H3DTexRes::ImgPixelStream, false, true);
   memset(data, 0, width * height * 4);
   h3dUnmapResStream(uiTexture_);

   std::ostringstream material;
   material << "<Material>" << std::endl;
   material << "   <Shader source=\"shaders/overlay.shader\"/>" << std::endl;
   material << "   <Sampler name=\"albedoMap\" map=\"" << h3dGetResName(uiTexture_) << "\" />" << std::endl;
   material << "</Material>" << std::endl;

   uiMatRes_ = h3dAddResource(H3DResTypes::Material, "UI Material", 0);
   bool result = h3dLoadResource(uiMatRes_, material.str().c_str(), material.str().length());
   ASSERT(result);
}