#ifndef _RADIANT_CLIENT_RENDERER_RENDER_COMPONENT_H
#define _RADIANT_CLIENT_RENDERER_RENDER_COMPONENT_H

#include "namespace.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

// xxx - Should we just get rid of this and use boost::any?  Eek!
class RenderComponent {
   public:
      virtual ~RenderComponent() { }
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDERER_RENDER_COMPONENT_H
 