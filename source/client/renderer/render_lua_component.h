#ifndef _RADIANT_CLIENT_RENDER_LUA_COMPONENT_H
#define _RADIANT_CLIENT_RENDER_LUA_COMPONENT_H

#include "namespace.h"
#include "om/om.h"
#include "render_component.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Renderer;

class RenderLuaComponent : public RenderComponent {
public:
   RenderLuaComponent(RenderEntity& entity, std::string const& name, om::DataStorePtr obj);
   ~RenderLuaComponent();

private:
   RenderEntity&        entity_;
   luabind::object      component_renderer_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_LUA_COMPONENT_H
