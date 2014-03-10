#include "pch.h"
#include "renderer.h"
#include "render_lua_component.h"
#include "resources/res_manager.h"
#include "om/components/data_store.ridl.h"

using namespace ::radiant;
using namespace ::radiant::client;

#define LC_LOG(level)      LOG(renderer.lua_component, level)

RenderLuaComponent::RenderLuaComponent(RenderEntity& entity, std::string const& name, luabind::object obj) :
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
         luabind::object ctor = script->RequireScript(path);
         ctor = luabind::object(script->GetCallbackThread(), ctor);

         try {
            std::weak_ptr<RenderEntity> re = entity.shared_from_this();
            obj_ = ctor(re);
            if (obj_) {
               update_fn_ = obj_["update"];
               Update(entity, obj);
            }
         } catch (std::exception const& e) {
            script->ReportCStackThreadException(ctor.interpreter(), e);
         }
      }
   }
}

void RenderLuaComponent::Update(RenderEntity& entity, luabind::object obj)
{
   try {
      if (update_fn_) {
         std::weak_ptr<RenderEntity> re = entity.shared_from_this();
         update_fn_(obj_, re, obj);
      }
   } catch (std::exception const& e) {
      lua::ScriptHost* script = Renderer::GetInstance().GetScriptHost();
      script->ReportCStackThreadException(update_fn_.interpreter(), e);
   }
}

RenderLuaComponent::~RenderLuaComponent()
{
   if (obj_) {
      lua::ScriptHost* script = Renderer::GetInstance().GetScriptHost();
      try {
         luabind::object fn = obj_["destroy"];
         if (fn) {
            fn(obj_);
         }
      } catch (std::exception const& e) {
         script->ReportCStackThreadException(obj_.interpreter(), e);
      }
   }
}
