#include "pch.h"
#include "renderer.h"
#include "render_lua_component.h"
#include "resources/res_manager.h"
#include "om/components/data_store.ridl.h"

using namespace ::radiant;
using namespace ::radiant::client;

RenderLuaComponent::RenderLuaComponent(RenderEntity& entity, std::string const& name, om::DataStorePtr ds) :
   entity_(entity)
{
   size_t offset = name.find(':');
   if (offset != std::string::npos) {
      std::string modname = name.substr(0, offset);
      std::string component_name = name.substr(offset + 1, std::string::npos);

      auto const& res = res::ResourceManager2::GetInstance();
      json::Node const& manifest = res.LookupManifest(modname);
      json::Node cr = manifest.get_node("component_renderers");
      std::string path = cr.get<std::string>(component_name, "");

      if (!path.empty()) {
         lua::ScriptHost* script = Renderer::GetInstance().GetScriptHost();
         luabind::object ctor = script->RequireScript(path);

         std::weak_ptr<RenderEntity> re = entity.shared_from_this();
         try {
            obj_ = script->CallFunction<luabind::object>(ctor, re, ds);
         } catch (std::exception const& e) {
            LOG(WARNING) << e.what();
         }
      }
   }
}

RenderLuaComponent::~RenderLuaComponent()
{
   if (obj_ && obj_.is_valid()) {
      lua::ScriptHost* script = Renderer::GetInstance().GetScriptHost();
      try {
         luabind::object fn = obj_["destroy"];
         if (fn && fn.is_valid()) {
            luabind::call_function<void>(fn, obj_);
         }
      } catch (std::exception const& e) {
         LOG(WARNING) << "error destroying component renderer: " << e.what();
      }
   }
}
