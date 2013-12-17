#ifndef _RADIANT_CLIENT_NAMESPACE_H
#define _RADIANT_CLIENT_NAMESPACE_H

#define BEGIN_RADIANT_CLIENT_NAMESPACE  namespace radiant { namespace client {
#define END_RADIANT_CLIENT_NAMESPACE    } }

BEGIN_RADIANT_CLIENT_NAMESPACE

// Forward Defines
class Renderer;
class PerfHud;
class RenderContext;
class Clock;

#define CLIENT_LOG(level)     LOG(client, level)

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_NAMESPACE_H
