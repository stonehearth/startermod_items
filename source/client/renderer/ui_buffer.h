#ifndef _RADIANT_CLIENT_RENDERER_UIBUFFER_H
#define _RADIANT_CLIENT_RENDERER_UIBUFFER_H

#include "namespace.h"
#include "Horde3D.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class UiBuffer
{
   public:
      UiBuffer();
      ~UiBuffer();

      void buffersWereCleared();
      void update(const char* buffer);
      void allocateBuffers(int width, int height);
      H3DRes getMaterial() const;

   public:


   private:
      NO_COPY_CONSTRUCTOR(UiBuffer);

   private:
      int               width_, height_;
      H3DRes            uiMatRes_;
      H3DRes            uiTexture_;
      H3DRes            uiPbo_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_RENDERER_UIBUFFER_H
