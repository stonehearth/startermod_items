#include "../pch.h"
#include "open.h"
#include "om/entity.h"
#include "om/selection.h"
#include "om/data_binding.h"
#include "lib/json/core_json.h"
#include "lib/rpc/lua_deferred.h"
#include "lib/rpc/reactor_deferred.h"
#include "lib/voxel/qubicle_brush.h"
#include "lua/script_host.h"
#include "client/client.h"
#include "client/xz_region_selector.h"
#include "client/renderer/renderer.h" // xxx: move to renderer::open when we move the renderer!
#include "client/renderer/render_entity.h" // xxx: move to renderer::open when we move the renderer!
#include "client/renderer/lua_render_entity.h" // xxx: move to renderer::open when we move the renderer!
#include "client/renderer/lua_renderer.h" // xxx: move to renderer::open when we move the renderer!
#include "client/renderer/pipeline.h" // xxx: move to renderer::open when we move the renderer!
#include "glfw3.h"

using namespace ::radiant;
using namespace ::radiant::client;
using namespace luabind;

H3DNodeUnique Client_CreateVoxelRenderNode(lua_State* L, 
                                           H3DNode parent,
                                           csg::Region3 const& model,
                                           std::string const& mode,
                                           std::string const& material_path)
{
   H3DNodeUnique model_node;

   H3DRes material = 0;
   if (!material_path.empty()) {
      material = h3dAddResource(H3DResTypes::Material, material_path.c_str(), 0);
   }
   if (mode == "blueprint") {
      std::vector<csg::Point3f> points = csg::mesh_tools().ConvertRegionToOutline(model);
      H3DNode s = h3dRadiantAddDebugShapes(parent, "foo");
      
      uint i, c = points.size();
      for (i = 0; i < c; i += 2) {
         h3dRadiantAddDebugLine(s, points[i], points[i+1], csg::Color4(0, 128, 220, 192));
      }
      h3dRadiantCommitDebugShape(s);
      if (material) {
         h3dSetNodeParamI(s, H3DMesh::MatResI, material);
      }
      model_node = H3DNodeUnique(s);
   } else {
      csg::mesh_tools::mesh mesh = csg::mesh_tools().ConvertRegionToMesh(model);

      H3DNode mesh_node;
      model_node = Pipeline::GetInstance().AddMeshNode(parent, mesh, &mesh_node);
      if (material) {
         h3dSetNodeParamI(mesh_node, H3DMesh::MatResI, material);
      }
   }
   return model_node;
}

om::EntityRef Client_CreateEmptyAuthoringEntity()
{
   return Client::GetInstance().CreateEmptyAuthoringEntity();
}

om::EntityRef Client_CreateAuthoringEntity(std::string const& uri)
{
   return Client::GetInstance().CreateAuthoringEntity(uri);
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
   /*
    * If someone on the lua side forgot to call ::destroy, go ahead and do it for them
    * now.  This may be late if we didn't get a full gc in between ticks, but better
    * later than never, right!
    */
   ~LuaCallback() {
      if (dtor_) {
         dtor_();
      }
   }

   /*
    * Called from the C++ side to notify the lua holder of the promise that something
    * has changed.  Lua clients and register for this callback using the progress
    * method on the promise.  Somethign like: promise::notify(function(...) ... end())
    */
   bool Notify(luabind::object o) {
      if (cb_.is_valid() && luabind::type(cb_) == LUA_TFUNCTION) {
         luabind::object handled = luabind::call_function<luabind::object>(cb_, o);
         return handled.is_valid() && type(handled) == LUA_TBOOLEAN && luabind::object_cast<bool>(handled);
      }
      return false;
   }
   
   /*
    * Called from the lua side via progress::destroy to notify the C++ side that they
    * can un-hook the source of Notify callbacks.
    */
   void Destroy() {
      cb_ = object();      // stop firing callbacks...
      if (dtor_) {
         dtor_();
         dtor_ = nullptr;
      }
   }

   /*
    * Called from the C++ side when the lua side invokes the promises' destroy
    * function.
    */
   void OnDestroy(std::function<void()> dtor) {
      dtor_ = dtor;
   }

   /*
    * Called from the lua side to express interest in changes to the source object.
    * Whenever 'something happens', the supplied callback will get called.
    */
   void Progress(luabind::object cb) {
      cb_ = cb;
   }


   luabind::object         cb_;
   std::function<void()>   dtor_;
};

struct CaptureInputPromise : public LuaCallback
{
};
struct TraceRenderFramePromise : public LuaCallback
{
};
struct SetCursorPromise : public LuaCallback
{
};
DECLARE_SHARED_POINTER_TYPES(CaptureInputPromise)
DECLARE_SHARED_POINTER_TYPES(TraceRenderFramePromise)
DECLARE_SHARED_POINTER_TYPES(SetCursorPromise)

template <typename T>
std::shared_ptr<T> Client_AllocObject()
{
   return Client::GetInstance().GetAuthoringStore().AllocObject<T>();
}

om::DataBindingPPtr Client_CreateDataStore(lua_State* L)
{
   // make sure we return the strong pointer version
   om::DataBindingPPtr db = Client_AllocObject<om::DataBindingP>();
   db->SetDataObject(newtable(L));
   return db;
}

bool Client_IsValidStandingRegion(lua_State* L, csg::Region3 const& r)
{
   return Client::GetInstance().GetOctTree().GetNavGrid().IsValidStandingRegion(r);
}

CaptureInputPromisePtr Client_CaptureInput(lua_State* L)
{
   Client& c = Client::GetInstance();

   CaptureInputPromisePtr callback = std::make_shared<CaptureInputPromise>();
   CaptureInputPromiseRef cb = callback;

   Client::InputHandlerId id = c.ReserveInputHandler();
   c.SetInputHandler(id, [cb, &c, id, L](Input const& i) {
      CaptureInputPromisePtr callback = cb.lock();
      if (!callback) {
         c.RemoveInputHandler(id);
         return false;
      }
      return callback->Notify(luabind::object(L, i));
   });

   callback->OnDestroy([&c, id]() {
      c.RemoveInputHandler(id);
   });

   return callback;
}

TraceRenderFramePromisePtr Client_TraceRenderFrame(lua_State* L)
{
   Client& c = Client::GetInstance();

   TraceRenderFramePromisePtr callback = std::make_shared<TraceRenderFramePromise>();
   TraceRenderFramePromiseRef cb = callback;

   Client::TraceRenderFrameId id = c.ReserveTraceRenderFrameHandler();
   c.SetTraceRenderFrameHandler(id, [cb, L](float frameTime) {
      TraceRenderFramePromisePtr callback = cb.lock();
      if (callback) {
         callback->Notify(luabind::object(L, frameTime));
      }
   });

   callback->OnDestroy([&c, id]() {
      c.RemoveTraceRenderFrameHandler(id);
   });

   return callback;
}

SetCursorPromisePtr Client_SetCursor(lua_State* L, std::string const& name)
{
   Client::CursorStackId id = Client::GetInstance().InstallCursor(name);
   SetCursorPromisePtr promise = std::make_shared<SetCursorPromise>();
   SetCursorPromiseRef p = promise;

   promise->OnDestroy([id]() {
      Client::GetInstance().RemoveCursor(id);
   });

   return promise;
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

static bool Client_IsKeyDown(int key)
{
   return glfwGetKey(glfwGetCurrentContext(), key) == GLFW_PRESS;
}

DEFINE_INVALID_JSON_CONVERSION(CaptureInputPromise);
DEFINE_INVALID_JSON_CONVERSION(TraceRenderFramePromise);
DEFINE_INVALID_JSON_CONVERSION(SetCursorPromise);

void lua::client::open(lua_State* L)
{
   LuaRenderer::RegisterType(L);
   LuaRenderEntity::RegisterType(L);

   module(L) [
      namespace_("_radiant") [
         namespace_("client") [
            def("create_empty_authoring_entity",   &Client_CreateEmptyAuthoringEntity),
            def("create_authoring_entity",         &Client_CreateAuthoringEntity),
            def("destroy_authoring_entity",        &Client_DestroyAuthoringEntity),
            def("create_render_entity",            &Client_CreateRenderEntity),
            def("capture_input",                   &Client_CaptureInput),
            def("query_scene",                     &Client_QueryScene),
            def("select_xz_region",                &Client_SelectXZRegion),
            def("trace_render_frame",              &Client_TraceRenderFrame),
            def("set_cursor",                      &Client_SetCursor),
            def("create_voxel_render_node",        &Client_CreateVoxelRenderNode),
            def("alloc_region",                    &Client_AllocObject<om::Region3Boxed>),
            def("create_data_store",               &Client_CreateDataStore),
            def("is_valid_standing_region",        &Client_IsValidStandingRegion),
            def("is_key_down",                     &Client_IsKeyDown),
            lua::RegisterTypePtr<CaptureInputPromise>()
               .def("on_input",          &CaptureInputPromise::Progress)
               .def("destroy",           &CaptureInputPromise::Destroy)
            ,
            lua::RegisterTypePtr<TraceRenderFramePromise>()
               .def("on_frame",          &TraceRenderFramePromise::Progress)
               .def("destroy",           &CaptureInputPromise::Destroy)
            ,
            lua::RegisterTypePtr<SetCursorPromise>()
               .def("destroy",           &CaptureInputPromise::Destroy)
            ,
            // xxx: Input, MouseInput, KeyboardInput, etc. should be in open_core.cpp, right?
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
               .def_readonly("dx",      &MouseInput::dx)
               .def_readonly("dy",      &MouseInput::dy)
               .def_readonly("wheel",   &MouseInput::wheel)
               .def_readonly("in_client_area", &MouseInput::in_client_area)
               .def("up",               &MouseEvent_GetUp)
               .def("down",             &MouseEvent_GetDown)
            ,
            lua::RegisterType<KeyboardInput>()
               .enum_("constants") [
                  value("LEFT_SHIFT",        GLFW_KEY_LEFT_SHIFT),
                  value("RIGHT_SHIFT",       GLFW_KEY_RIGHT_SHIFT),
                  value("LEFT_CTRL",         GLFW_KEY_LEFT_CONTROL),
                  value("RIGHT_CTRL",        GLFW_KEY_RIGHT_CONTROL),
                  value("LEFT_ALT",          GLFW_KEY_LEFT_ALT),
                  value("RIGHT_ALT",         GLFW_KEY_RIGHT_ALT)
               ]
               .def_readonly("key",    &KeyboardInput::key)
               .def_readonly("down",   &KeyboardInput::down)
            ,
            lua::RegisterType<RawInput>()
         ]
      ]
   ];
}
