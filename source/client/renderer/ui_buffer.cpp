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
   int lastBuff = curBuff_ - 1;
   if (lastBuff < 0) {
      lastBuff = MAX_BUFFERS - 1;
   }
   return uiMatRes_[lastBuff];
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

void* UiBuffer::getNextUiBuffer() const
{
   if (!uiPbo_[curBuff_]) {
      return nullptr;
   }

   return h3dMapResStream(uiPbo_[curBuff_], 0, 0, 0, false, true);
}

void UiBuffer::update(const csg::Region2& rgn, const radiant::uint32* buff)
{
   if (!uiPbo_[curBuff_]) {
      return;
   }
   auto bounds = rgn.GetBounds();

   perfmon::SwitchToCounter("system ui buffer copy to pbo");

   radiant::uint32 buffStart = (rgn.GetBounds().min.x + (width_ * rgn.GetBounds().min.y));
   radiant::uint32 *destBuff = (uint32 *)getNextUiBuffer();
   buff += buffStart;
   for (int i = 0; i < bounds.GetHeight(); i++) {
      memmove(destBuff, buff, bounds.GetWidth() * 4);
      destBuff += bounds.GetWidth();
      buff += width_;
   }

   perfmon::SwitchToCounter("unmap ui pbo");
   h3dUnmapResStream(uiPbo_[curBuff_], rgn.GetBounds().GetArea() * 4);

   perfmon::SwitchToCounter("copy ui pbo to ui texture") ;

   h3dCopyBufferToBuffer(uiPbo_[curBuff_], uiTexture_[curBuff_], bounds.min.x, bounds.min.y, bounds.GetWidth(), bounds.GetHeight());

   curBuff_ = (curBuff_ + 1) % MAX_BUFFERS;
}

bool UiBuffer::isUiMaterial(H3DRes r) const
{
   for (int i = 0; i < MAX_BUFFERS; i++)
   {
      if (r == uiMatRes_[i]) 
      {
         return true;
      }
   }
   return false;
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

      uiTexture_[i] = h3dCreateTexture(texName.c_str(), width, height, H3DFormats::List::TEX_BGRA8, H3DResFlags::NoTexMipmaps | H3DResFlags::NoFlush);
      unsigned char *data = (unsigned char *)h3dMapResStream(uiTexture_[i], H3DTexRes::ImageElem, 0, H3DTexRes::ImgPixelStream, false, true);
      memset(data, 0, width * height * 4);
      h3dUnmapResStream(uiTexture_[i], 0);

      std::ostringstream material;
      material << "<Material>" << std::endl;
      material << "   <Shader source=\"shaders/overlay.shader\"/>" << std::endl;
      material << "   <Sampler name=\"albedoMap\" map=\"" << texName << "\" />" << std::endl;
      material << "</Material>" << std::endl;

      uiMatRes_[i] = h3dAddResource(H3DResTypes::Material, matName.c_str(), H3DResFlags::NoFlush);
      bool result = h3dLoadResource(uiMatRes_[i], material.str().c_str(), material.str().length());
      ASSERT(result);
   }
}