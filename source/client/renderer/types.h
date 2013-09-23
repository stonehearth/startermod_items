#ifndef _RADIANT_CLIENT_RENDERER_TYPES_H
#define _RADIANT_CLIENT_RENDERER_TYPES_H

#include "namespace.h"
#include "Horde3D.h"
#include "Horde3DRadiant.h"
#include <boost/shared_ptr.hpp>

BEGIN_RADIANT_CLIENT_NAMESPACE

class RenderEntity;
typedef std::weak_ptr<RenderEntity> RenderEntityRef;
typedef std::shared_ptr<RenderEntity> RenderEntityPtr;

class RenderNode {
public:
   RenderNode() : node_(0) { }
   RenderNode(H3DNode node) : node_((void*)node, RemoveNode) {
   }

   operator H3DNode() {
      return (H3DNode)(node_.get());
   }

   RenderNode const& operator=(H3DNode node) {
      node_ = std::shared_ptr<void>((void*)node, RemoveNode);
      return *this;
   }

private:
   static void RemoveNode(void *n) {
      if (n) {
         h3dRemoveNode((H3DNode)n);
      }
   }

private:
   std::shared_ptr<void> node_;
};


END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_RENDERER_TYPES_H
