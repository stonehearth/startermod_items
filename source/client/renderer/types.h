#ifndef _RADIANT_CLIENT_RENDERER_TYPES_H
#define _RADIANT_CLIENT_RENDERER_TYPES_H

#include "namespace.h"
#include "Horde3D.h"
#include "Horde3DRadiant.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class RenderEntity;
typedef std::weak_ptr<RenderEntity> RenderEntityRef;
typedef std::shared_ptr<RenderEntity> RenderEntityPtr;

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_RENDERER_TYPES_H
