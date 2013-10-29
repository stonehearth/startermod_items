#include "../pch.h"
#include "open.h"
#include "om/entity.h"
#include "om/selection.h"
#include "om/data_binding.h"
#include "lib/json/core_json.h"
#include "lib/rpc/lua_deferred.h"
#include "lib/rpc/reactor_deferred.h"
#include "lib/voxel/qubicle_brush.h"
#include "lib/lua/script_host.h"
#include "client/client.h"
#include "client/xz_region_selector.h"
#include "client/renderer/renderer.h" // xxx: move to renderer::open when we move the renderer!
#include "client/renderer/render_entity.h" // xxx: move to renderer::open when we move the renderer!
#include "client/renderer/lua_render_entity.h" // xxx: move to renderer::open when we move the renderer!
#include "client/renderer/lua_renderer.h" // xxx: move to renderer::open when we move the renderer!
#include "client/renderer/pipeline.h" // xxx: move to renderer::open when we move the renderer!
#include "core/guard.h"
#include "core/slot.h"
#include "glfw3.h"

using namespace ::radiant;
using namespace ::radiant::client;
using namespace luabind;

static H3DNode CreateBlueprintOutline(H3DNode parent,
                                      float line_width,
                                      csg::Region3 const& model,
                                      std::string const& material_path)
{
   H3DNode mesh_node;
   csg::mesh_tools::mesh mesh = csg::mesh_tools().ConvertRegionToOutline(model, line_width, csg::Color3(255, 0, 0));

   H3DNodeUnique model_node = Pipeline::GetInstance().AddMeshNode(parent, mesh, &mesh_node);
   H3DRes material = h3dAddResource(H3DResTypes::Material, material_path.c_str(), 0);
   h3dSetNodeParamI(mesh_node, H3DMesh::MatResI, material);

   H3DNode node = model_node.get();
   model_node.release();

   return node;
}

static H3DNode CreateBlueprintPanels(H3DNode parent,
                                     float line_width,
                                     csg::Region3 const& model,
                                     std::string const& material_path)
{
   H3DNode mesh_node;
   csg::mesh_tools::mesh mesh = csg::mesh_tools().ConvertRegionToMesh(model);

#if 0
   csg::mesh_tools m;
   m.ForEachRegionPlane(model, 0, [&](csg::Region2 const& r2, csg::mesh_tools::PlaneInfo const& pi) {
      csg::Region2f inset = m.InsetRegion(r2, line_width);
      AddRegionToMesh(r2, pi, m);
   });
#endif

   H3DNodeUnique model_node = Pipeline::GetInstance().AddMeshNode(parent, mesh, &mesh_node);
   H3DRes material = h3dAddResource(H3DResTypes::Material, material_path.c_str(), 0);
   h3dSetNodeParamI(mesh_node, H3DMesh::MatResI, material);

   H3DNode node = model_node.get();
   model_node.release();

   return node;
}

H3DNodeUnique Client_CreateBlueprintNode(lua_State* L, 
                                         H3DNode parent,
                                         csg::Region3 const& model,
                                         std::string const& material_path)
{
   static const float line_width = 0.10f;
   static int unique = 1;

   H3DNode group = h3dAddGroupNode(parent, BUILD_STRING("blueprint node " << unique++).c_str());

   CreateBlueprintOutline(group, line_width, model, material_path);
   CreateBlueprintPanels(group, line_width, model, material_path);

   return group;
}


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
   csg::mesh_tools::mesh mesh;
   if (mode == "blueprint") {
      mesh = csg::mesh_tools().ConvertRegionToOutline(model, 0.15f, csg::Color3(255, 0, 0));
   } else {
      mesh = csg::mesh_tools().ConvertRegionToMesh(model);
   }
   H3DNode mesh_node;
   model_node = Pipeline::GetInstance().AddMeshNode(parent, mesh, &mesh_node);
   if (material) {
      h3dSetNodeParamI(mesh_node, H3DMesh::MatResI, material);
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

class CaptureInputPromise;
class TraceRenderFramePromise;
class SetCursorPromise;

DECLARE_SHARED_POINTER_TYPES(CaptureInputPromise)
DECLARE_SHARED_POINTER_TYPES(TraceRenderFramePromise)
DECLARE_SHARED_POINTER_TYPES(SetCursorPromise)

class CaptureInputPromise
{
public:
   CaptureInputPromise() {
      Client& c = Client::GetInstance();

      L_ = c.GetScriptHost()->GetCallbackThread();
      id_ = c.ReserveInputHandler();

      c.SetInputHandler(id_, [=](Input const& input) -> bool {
         if (cb_.is_valid()) {
            return luabind::call_function<bool>(cb_, input);
         }
         return false;
      });
   }

   ~CaptureInputPromise() { 
      Destroy();
   }

   void OnInput(luabind::object cb) {
      cb_ = luabind::object(L_, cb);
   }
   void Destroy() {
      Client::GetInstance().RemoveInputHandler(id_);
      id_ = 0;
   }

private:
   lua_State*                 L_;
   Client::InputHandlerId     id_;
   luabind::object            cb_;
   core::Guard                input_guards_;
};

class TraceRenderFramePromise : public std::enable_shared_from_this<TraceRenderFramePromise>
{
public:
   TraceRenderFramePromise() {
      L_ = Client::GetInstance().GetScriptHost()->GetCallbackThread();
      guards_ += Renderer::GetInstance().OnRenderFrameStart([this](FrameStartInfo const& info) {
         frame_start_slot_.Signal(info);
      });
      guards_ += Renderer::GetInstance().OnServerTick([this](int now) {
         server_tick_slot_.Signal(now);
      });
   }

   ~TraceRenderFramePromise() {
   }

   TraceRenderFramePromisePtr OnFrameStart(luabind::object cb) {
      luabind::object callback(L_, cb);
      guards_ += frame_start_slot_.Register([=](FrameStartInfo const &info) {
         luabind::call_function<void>(callback, info.now, info.interpolate);
      });
      return shared_from_this();
   }

   TraceRenderFramePromisePtr OnServerTick(luabind::object cb) {
      luabind::object callback(L_, cb);
      guards_ += server_tick_slot_.Register([=](int now) {
         luabind::call_function<void>(callback, now);
      });
      return shared_from_this();
   }

   TraceRenderFramePromisePtr Destroy() {
      guards_.Clear();
      return shared_from_this();
   }

private:
   lua_State*                    L_;
   core::Slot<FrameStartInfo>    frame_start_slot_;
   core::Slot<int>               server_tick_slot_;
   core::Guard                   guards_;
};

class SetCursorPromise
{
public:
   SetCursorPromise(Client::CursorStackId id) : id_(id) { }

   ~SetCursorPromise() {
      Destroy();
   }

   void Destroy() {
      Client::GetInstance().RemoveCursor(id_);
   }

private:
   Client::CursorStackId      id_;
};

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
   return std::make_shared<CaptureInputPromise>();
}

TraceRenderFramePromisePtr Client_TraceRenderFrame(lua_State* L)
{
   return std::make_shared<TraceRenderFramePromise>();
}

SetCursorPromisePtr Client_SetCursor(lua_State* L, std::string const& name)
{
   Client::CursorStackId id = Client::GetInstance().InstallCursor(name);
   return std::make_shared<SetCursorPromise>(id);
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
IMPLEMENT_TRIVIAL_TOSTRING(CaptureInputPromise)
IMPLEMENT_TRIVIAL_TOSTRING(TraceRenderFramePromise)
IMPLEMENT_TRIVIAL_TOSTRING(SetCursorPromise)
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
            def("create_blueprint_node",           &Client_CreateBlueprintNode),
            def("alloc_region",                    &Client_AllocObject<om::Region3Boxed>),
            def("create_data_store",               &Client_CreateDataStore),
            def("is_valid_standing_region",        &Client_IsValidStandingRegion),
            def("is_key_down",                     &Client_IsKeyDown),
            lua::RegisterTypePtr<CaptureInputPromise>()
               .def("on_input",          &CaptureInputPromise::OnInput)
               .def("destroy",           &CaptureInputPromise::Destroy)
            ,
            lua::RegisterTypePtr<TraceRenderFramePromise>()
               .def("on_servet_tick",    &TraceRenderFramePromise::OnServerTick)
               .def("on_frame_start",    &TraceRenderFramePromise::OnFrameStart)
               .def("destroy",           &TraceRenderFramePromise::Destroy)
            ,
            lua::RegisterTypePtr<SetCursorPromise>()
               .def("destroy",           &SetCursorPromise::Destroy)
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
