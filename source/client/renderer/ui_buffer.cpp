#include "pch.h"
#include "ui_buffer.h"
#include "radiant.h"
#include "radiant_stdutil.h"
#include "Horde3DUtils.h"
#include "Horde3DRadiant.h"
#include "lib/perfmon/perfmon.h"

using namespace ::radiant;
using namespace ::radiant::client;

#define UB_LOG(level)      LOG(renderer.ui_buffer, level)

UiBuffer::UiBuffer() :
   size_(csg::Point2::zero)
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


void UiBuffer::update(csg::Point2 const& size, csg::Region2 const& rgn, const radiant::uint32* buff)
{
   perfmon::SwitchToCounter("system ui buffer copy to pbo");

   ASSERT(size.x && size.y);

   csg::Rect2 bounds = rgn.GetBounds();

   if (bounds.min.x < 0 || bounds.min.y < 0 || bounds.max.x > size.x || bounds.max.y > size.y) {
      UB_LOG(1) << "ignoring to update ui buffer with invalid bounds:" << bounds << " size:" << size;
      return;
   }
   UB_LOG(3) << "updating ui buffer (size:" << size << " rgnbounds:" << bounds << ")";

   resizeBuffers(size);

   if (!uiPbo_[curBuff_]) {
      UB_LOG(1) << "no pbo exists for buffer " << curBuff_ << ".  aborting.";
      return;
   }

   radiant::uint32 buffStart = (bounds.min.x + (size_.x * bounds.min.y));
   radiant::uint32 *destBuff = (uint32 *)h3dMapResStream(uiPbo_[curBuff_], 0, 0, 0, false, true);

   if (!destBuff) {
      UB_LOG(1) << "failed to map ui buffer " << curBuff_ << ".  aborting.";
      return;
   }

   UB_LOG(5) << "copying backing buffer to pbo (dst:" << destBuff << " src:" << buff << ")";

   int width = bounds.GetWidth();
   int height = bounds.GetHeight();

   buff += buffStart;
   destBuff += buffStart;

   if (LOG_LEVEL(renderer.ui_buffer) >= 5) {
      volatile static int read;
      UB_LOG(5) << "reading 1st byte of src:" << buff;
      read = *buff;

      UB_LOG(5) << "touching 1st byte of dst:" << destBuff;
      *destBuff = 0;

      UB_LOG(5) << "copying 1st scanline.";
      memmove(destBuff, buff, width * 4);

      UB_LOG(5) << "proceeding into loop.";
   }

   for (int i = 0; i < height; i++) {
      memmove(destBuff, buff, width * 4);
      destBuff += size_.x;
      buff += size_.x;
   }
   UB_LOG(5) << "finished copying backing buffer to pbo";

   perfmon::SwitchToCounter("unmap ui pbo");
   h3dUnmapResStream(uiPbo_[curBuff_], 0);
   UB_LOG(5) << "finished unmapping stream";

   perfmon::SwitchToCounter("copy ui pbo to ui texture") ;
   h3dCopyBufferToBuffer(uiPbo_[curBuff_], uiTexture_[curBuff_], bounds.min.x, bounds.min.y, bounds.GetWidth(), bounds.GetHeight());
   UB_LOG(5) << "finished copying pbo to texture";

   curBuff_ = (curBuff_ + 1) % MAX_BUFFERS;
}

void UiBuffer::resizeBuffers(csg::Point2 const& size)
{
   if (size == size_) {
      return;
   }
   UB_LOG(3) << "resizing ui buffer to " << size << " (old:" << size_ << ").";
   size_ = size;
   reallocateBuffers();
}

void UiBuffer::reallocateBuffers()
{
   if (size_ == csg::Point2::zero) {
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

      int dataSize = size_.x * size_.y * 4;
      uiPbo_[i] = h3dCreatePixelBuffer(pboName.c_str(), dataSize);

      uiTexture_[i] = h3dCreateTexture(texName.c_str(), size_.x, size_.y, H3DFormats::List::TEX_BGRA8, H3DResFlags::NoTexMipmaps | H3DResFlags::NoFlush);
      unsigned char *data = (unsigned char *)h3dMapResStream(uiTexture_[i], H3DTexRes::ImageElem, 0, H3DTexRes::ImgPixelStream, false, true);
      if (!data) {
         UB_LOG(1) << "failed to map texture while allocating ui buffer of size " << size_;
         return;
      }
      memset(data, 0, dataSize);
      h3dUnmapResStream(uiTexture_[i], 0);

      std::ostringstream material;
      material << "{" << std::endl;
      material << "   \"shaders\": {" << std::endl;
      material << "       \"OVERLAY\" : {" << std::endl;
      material << "           \"state\" : \"states/voxel/blend.state\"," << std::endl;
      material << "           \"shader\" : \"shaders/overlay.shader\"" << std::endl;
      material << "        }" << std::endl;
      material << "    }," << std::endl;
      material << "   \"samplers\" : [" << std::endl;
      material << "       {" << std::endl;
      material << "           \"name\" : \"albedoMap\"," << std::endl;
      material << "           \"map\" : \"" << texName << "\"" << std::endl;
      material << "       }" << std::endl;
      material << "    ]" << std::endl;
      material << "}" << std::endl;




      uiMatRes_[i] = h3dAddResource(H3DResTypes::Material, matName.c_str(), H3DResFlags::NoFlush);
      bool result = h3dLoadResource(uiMatRes_[i], material.str().c_str(), (int)material.str().length());
      ASSERT(result);
   }
}