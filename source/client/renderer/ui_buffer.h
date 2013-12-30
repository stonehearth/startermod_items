#ifndef _RADIANT_CLIENT_RENDERER_UIBUFFER_H
#define _RADIANT_CLIENT_RENDERER_UIBUFFER_H

#include "namespace.h"
#include "Horde3D.h"

#define MAX_BUFFERS 1

BEGIN_RADIANT_CLIENT_NAMESPACE

class UiBuffer
{
   public:
      UiBuffer();
      ~UiBuffer();

      void buffersWereCleared();
      void update();
      void allocateBuffers(int width, int height);
      H3DRes getMaterial() const;
      bool isUiMaterial(H3DRes m) const;
      void* getNextUiBuffer() const;
      void* getLastUiBuffer() const;

   private:
      NO_COPY_CONSTRUCTOR(UiBuffer);

   private:
      int               curBuff_;
      int               width_, height_;
      H3DRes            uiMatRes_[MAX_BUFFERS];
      H3DRes            uiTexture_[MAX_BUFFERS];
      H3DRes            uiPbo_[MAX_BUFFERS];
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_RENDERER_UIBUFFER_H
