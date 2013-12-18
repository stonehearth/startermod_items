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
   width_(0),
   height_(0)
{
   for (int i = 0; i < MAX_BUFFERS; i++) {
      uiTexture_[i] = 0x0;
      uiMatRes_[i] = 0x0;
      uiPbo_[i] = 0x0;
   }
   curBuff_ = 0;
}

UiBuffer::~UiBuffer()
{

}

H3DRes UiBuffer::getMaterial() const 
{
   return uiMatRes_[curBuff_];
}

void UiBuffer::buffersWereCleared()
{
   for (int i = 0; i < MAX_BUFFERS; i++) {
      uiTexture_[i] = 0x0;
      uiMatRes_[i] = 0x0;
      uiPbo_[i] = 0x0;
   }
   curBuff_ = 0;

   allocateBuffers(width_, height_);
}

void UiBuffer::update(const char* buffer)
{
   if (!uiPbo_[curBuff_]) {
      return;
   }

   perfmon::SwitchToCounter("map ui pbo");

   char *data = (char *)h3dMapResStream(uiPbo_[curBuff_], 0, 0, 0, false, true);

   if (data) {
      perfmon::SwitchToCounter("copy client mem to ui pbo");
      // If you think this is slow (bliting everything instead of just the dirty rects), please
      // talk to Klochek; the explanation is too large to fit in the margins of this code....
      memcpy(data, buffer, width_ * height_ * 4);
   }
   perfmon::SwitchToCounter("unmap ui pbo");
   h3dUnmapResStream(uiPbo_[curBuff_]);

   perfmon::SwitchToCounter("copy ui pbo to ui texture") ;

   h3dCopyBufferToBuffer(uiPbo_[curBuff_], uiTexture_[curBuff_], 0, 0, width_, height_);

   curBuff_ = (curBuff_ + 1) % MAX_BUFFERS;
}

void UiBuffer::allocateBuffers(int width, int height)
{
   if (width != width_ || height != height_) {
      width_ = width;
      height_ = height;
   } else {
      return;
   }

   for (int i = 0; i < MAX_BUFFERS; i++)
   {
      if (uiPbo_[i]) {
         h3dRemoveResource(uiPbo_[i]);
         uiPbo_[i] = 0x0;
      }
      if (uiTexture_[i]) {
         h3dRemoveResource(uiTexture_[i]);
         uiTexture_[i] = 0x0;
      }
      if (uiMatRes_[i]) {
         h3dUnloadResource(uiMatRes_[i]);
         uiMatRes_[i] = 0x0;

      }
      h3dReleaseUnusedResources();
      std::string loopVal = std::to_string(i);
      std::string pboName("ui_pbo " + loopVal);
      std::string texName("ui_texture " + loopVal);
      std::string matName("ui_mat " + loopVal);

      uiPbo_[i] = h3dCreatePixelBuffer(pboName.c_str(), width * height * 4);

      uiTexture_[i] = h3dCreateTexture(texName.c_str(), width, height, H3DFormats::List::TEX_BGRA8, H3DResFlags::NoTexMipmaps);
      unsigned char *data = (unsigned char *)h3dMapResStream(uiTexture_[i], H3DTexRes::ImageElem, 0, H3DTexRes::ImgPixelStream, false, true);
      memset(data, 0, width * height * 4);
      h3dUnmapResStream(uiTexture_[i]);

      std::ostringstream material;
      material << "<Material>" << std::endl;
      material << "   <Shader source=\"shaders/overlay.shader\"/>" << std::endl;
      material << "   <Sampler name=\"albedoMap\" map=\"" << texName << "\" />" << std::endl;
      material << "</Material>" << std::endl;

      uiMatRes_[i] = h3dAddResource(H3DResTypes::Material, matName.c_str(), 0);
      bool result = h3dLoadResource(uiMatRes_[i], material.str().c_str(), material.str().length());
      ASSERT(result);
   }
}