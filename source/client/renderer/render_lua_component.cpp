#include "pch.h"
#include "renderer.h"
#include "render_lua_component.h"
#include "resources/res_manager.h"
#include "om/components/data_store.ridl.h"

using namespace ::radiant;
using namespace ::radiant::client;

#define LC_LOG(level)      LOG(renderer.lua_component, level)

RenderLuaComponent::RenderLuaComponent(RenderEntity& entity, std::string const& name, om::DataStorePtr datastore) :
   entity_(entity)
{
   size_t offset = name.find(':');
   if (offset != std::string::npos) {
      std::string modname = name.substr(0, offset);
      std::string component_name = name.substr(offset + 1, std::string::npos);

      auto const& res = res::ResourceManager2::GetInstance();
      std::string path;
      res.LookupManifest(modname, [&](const res::Manifest &manifest) {
         json::Node cr = manifest.get_node("component_renderers");
         path = cr.get<std::string>(component_name, "");
      });

      if (!path.empty()) {
         lua::ScriptHost* script = Renderer::GetInstance().GetScriptHost();
         lua_State* L = script->GetCallbackThread();
         try {
            luabind::object ctor = script->RequireScript(path);
            ctor = luabind::object(L, ctor);

            std::weak_ptr<RenderEntity> re = entity.shared_from_this();
            component_renderer_ = ctor();
            luabind::object fn = component_renderer_["initialize"];
            if (fn) {
               luabind::object controller = datastore->GetController();
               if (!controller) {
                  controller = luabind::object(L, datastore);
               }
               fn(component_renderer_, re, controller);
            }
         } catch (std::exception const& e) {
            script->ReportCStackThreadException(L, e);
         }
      }
   }
}

RenderLuaComponent::~RenderLuaComponent()
{
   if (component_renderer_) {
      lua::ScriptHost* script = Renderer::GetInstance().GetScriptHost();
      try {
         luabind::object fn = component_renderer_["destroy"];
         if (fn) {
            fn(component_renderer_);
         }
      } catch (std::exception const& e) {
         script->ReportCStackThreadException(component_renderer_.interpreter(), e);
      }
   }
}
