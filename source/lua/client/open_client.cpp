#include "../pch.h"
#include "open.h"
#include "om/entity.h"
#include "om/selection.h"
#include "lib/rpc/lua_deferred.h"
#include "lib/rpc/reactor_deferred.h"
#include "lua/script_host.h"
#include "client/client.h"
#include "client/xz_region_selector.h"
#include "client/renderer/renderer.h" // xxx: move to renderer::open when we move the renderer!
#include "client/renderer/render_entity.h" // xxx: move to renderer::open when we move the renderer!
#include "client/renderer/lua_render_entity.h" // xxx: move to renderer::open when we move the renderer!
#include "client/renderer/lua_renderer.h" // xxx: move to renderer::open when we move the renderer!

using namespace ::radiant;
using namespace ::radiant::client;
using namespace luabind;

om::EntityRef Client_CreateEmptyAuthoringEntity()
{
   return Client::GetInstance().CreateEmptyAuthoringEntity();
}

om::EntityRef Client_CreateAuthoringEntity(std::string const& mod_name, std::string const& entity_name)
{
   return Client::GetInstance().CreateAuthoringEntity(mod_name, entity_name);
}

om::EntityRef Client_CreateAuthoringEntityByRef(std::string const& ref)
{
   return Client::GetInstance().CreateAuthoringEntityByRef(ref);
}

void Client_DestroyAuthoringEntity(dm::ObjectId id)
{
   return Client::GetInstance().DestroyAuthoringEntity(id);
}

std::weak_ptr<RenderEntity> Client_CreateRenderEntity(H3DNode parent, luabind::object arg)
{
   om::EntityPtr entity;
   std::weak_ptr<RenderEntity> result;

   if (luabind::type(arg) == LUA_TSTRING) {
      // arg is a path to an object (e.g. /objects/3).  If this leads to a Entity, we're all good
      std::string path = luabind::object_cast<std::string>(arg);
      dm::Store& store = Client::GetInstance().GetStore();
      dm::ObjectPtr obj =  om::ObjectFormatter().GetObject(store, path);
      if (obj && obj->GetObjectType() == om::Entity::DmType) {
         entity = std::static_pointer_cast<om::Entity>(obj);
      }
   } else {
      try {
         entity = luabind::object_cast<om::EntityRef>(arg).lock();
      } catch (luabind::cast_failed&) {
      }
   }
   if (entity) {
      result = Renderer::GetInstance().CreateRenderObject(parent, entity);
   }
   return result;
}

static luabind::object
Client_QueryScene(lua_State* L, int x, int y)
{
   om::Selection s;
   Renderer::GetInstance().QuerySceneRay(x, y, s);

   using namespace luabind;
   object result = newtable(L);
   if (s.HasBlock()) {
      result["location"] = object(L, s.GetBlock());
      result["normal"]   = object(L, csg::ToInt(s.GetNormal()));
   }
   return result;
}

rpc::LuaDeferredPtr Client_SelectXZRegion(lua_State* L)
{
   Client &c = Client::GetInstance();
   auto d = std::make_shared<XZRegionSelector>(c.GetTerrain())->Activate();

   rpc::LuaDeferredPtr result = std::make_shared<rpc::LuaDeferred>("select xz region");
   d->Progress([L, result](csg::Cube3 const& c) {
      result->Notify(luabind::object(L, c));
   });
   d->Done([L, result](csg::Cube3 const& c) {
      result->Resolve(luabind::object(L, c));
   });
   d->Fail([L, result, d](std::string const& error) {
      result->Reject(luabind::object(L, error));
   });

   return result;
}

struct LuaCallback {
   bool Notify(luabind::object o) {
      if (cb_.is_valid() && luabind::type(cb_) == LUA_TFUNCTION) {
         luabind::object handled = luabind::call_function<luabind::object>(cb_, o);
         return handled.is_valid() && type(handled) == LUA_TBOOLEAN && luabind::object_cast<bool>(handled);
      }
      return false;
   }

   void Progress(luabind::object cb) {
      cb_ = cb;
   }

   luabind::object      cb_;
};
DECLARE_SHARED_POINTER_TYPES(LuaCallback)

LuaCallbackPtr Client_CaptureInput(lua_State* L)
{
   Client& c = Client::GetInstance();

   LuaCallbackPtr callback = std::make_shared<LuaCallback>();
   LuaCallbackRef cb = callback;

   Client::InputHandlerId id = c.ReserveInputHandler();
   c.SetInputHandler(id, [cb, &c, id, L](Input const& i) {
      LuaCallbackPtr callback = cb.lock();
      if (!callback) {
         c.RemoveInputHandler(id);
         return false;
      }
      return callback->Notify(luabind::object(L, i));
   });
   return callback;
}

static bool MouseEvent_GetUp(MouseInput const& me, int button)
{
   button--;
   if (button >= 0 && button < sizeof(me.up)) {
      return me.up[button];
   }
   return false;
}

static bool MouseEvent_GetDown(MouseInput const& me, int button)
{
   button--;
   if (button >= 0 && button < sizeof(me.down)) {
      return me.down[button];
   }
   return false;
}

IMPLEMENT_TRIVIAL_TOSTRING(Client)
IMPLEMENT_TRIVIAL_TOSTRING(LuaCallback)
IMPLEMENT_TRIVIAL_TOSTRING(Input)
IMPLEMENT_TRIVIAL_TOSTRING(MouseInput)
IMPLEMENT_TRIVIAL_TOSTRING(KeyboardInput)
IMPLEMENT_TRIVIAL_TOSTRING(RawInput)

void lua::client::open(lua_State* L)
{
   LuaRenderer::RegisterType(L);
   LuaRenderEntity::RegisterType(L);

   module(L) [
      namespace_("_radiant") [
         namespace_("client") [
            def("create_empty_authoring_entity",   &Client_CreateEmptyAuthoringEntity),
            def("create_authoring_entity",         &Client_CreateAuthoringEntity),
            def("create_authoring_entity_by_ref",  &Client_CreateAuthoringEntityByRef),
            def("destroy_authoring_entity",        &Client_DestroyAuthoringEntity),
            def("create_render_entity",            &Client_CreateRenderEntity),
            def("capture_input",                   &Client_CaptureInput),
            def("query_scene",                     &Client_QueryScene),
            def("select_xz_region",                &Client_SelectXZRegion),
            lua::RegisterTypePtr<LuaCallback>()
               .def("on_input",           &LuaCallback::Progress)
            ,
            lua::RegisterType<Input>()
               .enum_("constants") [
                  value("MOUSE", Input::MOUSE),
                  value("KEYBOARD", Input::KEYBOARD),
                  value("RAW_INPUT", Input::RAW_INPUT)
               ]
               .def_readonly("type",      &Input::type)
               .def_readonly("mouse",     &Input::mouse)
               .def_readonly("keyboard",  &Input::keyboard)
               .def_readonly("raw_input", &Input::raw_input)
            ,
            lua::RegisterType<MouseInput>()
               .def_readonly("x",       &MouseInput::x)
               .def_readonly("y",       &MouseInput::y)
               .def_readonly("wheel",   &MouseInput::wheel)
               .def("up",               &MouseEvent_GetUp)
               .def("down",             &MouseEvent_GetDown)
            ,
            lua::RegisterType<KeyboardInput>()
            ,
            lua::RegisterType<RawInput>()
         ]
      ]
   ];
}
