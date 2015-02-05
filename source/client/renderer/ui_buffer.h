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

      void update(csg::Point2 const& size, csg::Region2 const& rgn, const uint32* buff);
      H3DRes getMaterial() const;

   private:
      NO_COPY_CONSTRUCTOR(UiBuffer);
      void resizeBuffers(csg::Point2 const& size);
      void reallocateBuffers();

   private:
      int               curBuff_;
      csg::Point2       size_;
      H3DRes            uiMatRes_[MAX_BUFFERS];
      H3DRes            uiTexture_[MAX_BUFFERS];
      H3DRes            uiPbo_[MAX_BUFFERS];
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_RENDERER_UIBUFFER_H
