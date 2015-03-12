#ifndef _RADIANT_CLIENT_NAMESPACE_H
#define _RADIANT_CLIENT_NAMESPACE_H

#define BEGIN_RADIANT_CLIENT_NAMESPACE  namespace radiant { namespace client {
#define END_RADIANT_CLIENT_NAMESPACE    } }

BEGIN_RADIANT_CLIENT_NAMESPACE

// Forward Defines
class Renderer;
class PerfHud;
class FlameGraphHud;
class RenderContext;
class Clock;
class RenderNode;
class RenderTerrain;
class RenderTerrainTile;
class RenderTerrainLayer;

DECLARE_SHARED_POINTER_TYPES(RenderNode)

#define CLIENT_LOG(level)     LOG(client.core, level)

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_NAMESPACE_H
