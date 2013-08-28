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
   RenderNode(H3DNode node) : node_((void*)node, RemoveNode) { }

   operator H3DNode() {
      return (H3DNode)(node_.get());
   }

private:
   static void RemoveNode(void *n) {
      h3dRemoveNode((H3DNode)n);
   }

private:
   std::shared_ptr<void> node_;
};


typedef std::shared_ptr<H3DNode> RenderH3dNode;


END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_RENDERER_TYPES_H
