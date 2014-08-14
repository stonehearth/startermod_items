#include "../pch.h"
#include "open.h"
#include "om/entity.h"
#include "om/selection.h"
#include "om/components/data_store.ridl.h"
#include "lib/json/core_json.h"
#include "lib/rpc/lua_deferred.h"
#include "lib/rpc/reactor_deferred.h"
#include "lib/voxel/qubicle_brush.h"
#include "lib/lua/script_host.h"
#include "lib/perfmon/perfmon.h"
#include "client/client.h"
#include "client/xz_region_selector.h"
#include "client/renderer/renderer.h" // xxx: move to renderer::open when we move the renderer!
#include "client/renderer/render_entity.h" // xxx: move to renderer::open when we move the renderer!
#include "client/renderer/lua_render_entity.h" // xxx: move to renderer::open when we move the renderer!
#include "client/renderer/lua_renderer.h" // xxx: move to renderer::open when we move the renderer!
#include "client/renderer/pipeline.h" // xxx: move to renderer::open when we move the renderer!
#include "core/guard.h"
#include "core/slot.h"
#include "csg/region_tools.h"
#include "glfw3.h"
#include "client/renderer/raycast_result.h"

using namespace ::radiant;
using namespace ::radiant::client;
using namespace luabind;

luabind::object Client_GetObject(lua_State* L, object id)
{
   Client &client = Client::GetInstance();
   dm::ObjectPtr obj;
   luabind::object lua_obj;

   int id_type = type(id);

   if (id_type == LUA_TNUMBER) {
      dm::ObjectId object_id = object_cast<int>(id);
      obj = client.GetStore().FetchObject<dm::Object>(object_id);
   } else if (id_type == LUA_TSTRING) {
      const char* addr = object_cast<const char*>(id);
      obj = client.GetStore().FetchObject<dm::Object>(addr);
      if (!obj) {
         obj = client.GetAuthoringStore().FetchObject<dm::Object>(addr);
      }
   }
   if (obj) {
      lua::ScriptHost* host = lua::ScriptHost::GetScriptHost(L);
      lua_obj = host->CastObjectToLua(obj);
   }
   return lua_obj;
}

void Client_SelectEntity(lua_State* L, luabind::object o)
{
   om::EntityPtr entity;
   boost::optional<om::EntityRef> e = object_cast_nothrow<om::EntityRef>(o);
   if (e.is_initialized()) {
      entity = e.get().lock();
   }
   Client::GetInstance().SelectEntity(entity);
}

void Client_HilightEntity(lua_State* L, luabind::object o)
{
   om::EntityPtr entity;
   boost::optional<om::EntityRef> e = object_cast_nothrow<om::EntityRef>(o);
   if (e.is_initialized()) {
      entity = e.get().lock();
   }
   Client::GetInstance().HilightEntity(entity);
}

RenderNodePtr Client_CreateVoxelNode(lua_State* L, 
                                     H3DNode parent,
                                     csg::Region3 const& model,
                                     std::string const& material_path,
                                     csg::Point3f const& origin)
{
   csg::mesh_tools::mesh mesh;
   csg::RegionToMesh(model, mesh, -origin, false);

   return RenderNode::CreateCsgMeshNode(parent, mesh)
                           ->SetMaterial(material_path);
}


RenderNodePtr Client_CreateObjRenderNode(lua_State* L, 
                                         RenderNodePtr const& parent,
                                         std::string const& objfile)
   {
   return RenderNode::CreateObjNode(parent->GetNode(), objfile);
}

RenderNodePtr Client_CreateQubicleMatrixNode(lua_State* L, 
                                          H3DNode parent,
                                          std::string const& qubicle_file,
                                          std::string const& qubicle_matrix,
                                          csg::Point3f const& origin)
{
   RenderNodePtr node;
   Pipeline& pipeline = Pipeline::GetInstance();

   voxel::QubicleFile const* qubicle = res::ResourceManager2::GetInstance().OpenQubicleFile(qubicle_file);
   if (qubicle) {
      voxel::QubicleMatrix const* matrix = qubicle->GetMatrix(qubicle_matrix);

      if (matrix) {
         ResourceCacheKey key;
         key.AddElement("origin", origin);
         key.AddElement("matrix", matrix);

         // xxx: can we create a shared mesh without lod levels?
         auto create_mesh = [&matrix, &origin](csg::mesh_tools::mesh &mesh, int lodLevel) {
            csg::Region3 model = voxel::QubicleBrush(matrix)
               .SetOffsetMode(voxel::QubicleBrush::Matrix)
               .PaintOnce();
            csg::RegionToMesh(model, mesh, -origin, true);
         };
         node = RenderNode::CreateSharedCsgMeshNode(parent, key, create_mesh);
      }
   }
   return node;
}

RenderNodePtr Client_CreateDesignationNode(lua_State* L, 
                                     H3DNode parent,
                                     csg::Region2 const& model,
                                     csg::Color4 const& outline,
                                     csg::Color4 const& stripes)
{
   return Pipeline::GetInstance().CreateDesignationNode(parent, model, outline, stripes);
}

RenderNodePtr Client_CreateSelectionNode(lua_State* L, 
                                  H3DNode parent,
                                  csg::Region2 const& model,
                                  csg::Color4 const& interior_color,
                                  csg::Color4 const& border_color)
{
   return Pipeline::GetInstance().CreateSelectionNode(parent, model, interior_color, border_color);
}

RenderNodePtr Client_CreateStockpileNode(lua_State* L, 
                                   H3DNode parent,
                                   csg::Region2 const& model,
                                   csg::Color4 const& interior_color,
                                   csg::Color4 const& border_color)
{
   return Pipeline::GetInstance().CreateStockpileNode(parent, model, interior_color, border_color);
}

om::EntityRef Client_CreateAuthoringEntity(std::string const& uri)
{
   return Client::GetInstance().CreateAuthoringEntity(uri);
}

void Client_DestroyAuthoringEntity(dm::ObjectId id)
{
   return Client::GetInstance().DestroyAuthoringEntity(id);
}

std::weak_ptr<RenderEntity> Client_GetRenderEntity(luabind::object arg)
{
   om::EntityPtr entity;
   std::weak_ptr<RenderEntity> result;
   if (luabind::type(arg) == LUA_TSTRING) {
      // arg is a path to an object (e.g. /objects/3).  If this leads to a Entity, we're all good
      std::string path = luabind::object_cast<std::string>(arg);
      dm::Store& store = Client::GetInstance().GetStore();
      dm::ObjectPtr obj = store.FetchObject<dm::Object>(path);
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
      result = Renderer::GetInstance().GetRenderEntity(entity);
   }
   return result;
}

std::weak_ptr<RenderEntity> Client_CreateRenderEntity(H3DNode parent, luabind::object arg)
{
   om::EntityPtr entity;
   std::weak_ptr<RenderEntity> result;
   if (luabind::type(arg) == LUA_TSTRING) {
      // arg is a path to an object (e.g. /objects/3).  If this leads to a Entity, we're all good
      std::string path = luabind::object_cast<std::string>(arg);
      dm::Store& store = Client::GetInstance().GetStore();
      dm::ObjectPtr obj = store.FetchObject<dm::Object>(path);
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
      auto existing_re = Renderer::GetInstance().GetRenderEntity(entity);

      if (!existing_re) {
         std::shared_ptr<RenderEntity> re = Renderer::GetInstance().CreateRenderEntity(parent, entity);
         re->SetParentOverride(true);
         result = re;
      } else {
         result = existing_re;
      }
   }
   return result;
}


std::weak_ptr<RenderEntity> Client_CreateRenderEntityUnparented(luabind::object arg)
{
   return Client_CreateRenderEntity(RenderNode::GetUnparentedNode(), arg);
}

static RaycastResult
Client_QueryScene(lua_State* L, int x, int y)
{
   return Renderer::GetInstance().QuerySceneRay(x, y, 0);
}

XZRegionSelectorPtr Client_CreateXZRegionSelector()
{
   XZRegionSelectorPtr foo = Client::GetInstance().CreateXZRegionSelector();
   return foo;
}

rpc::LuaDeferredPtr Client_ActivateXZRegionSelector(lua_State* L, XZRegionSelectorPtr selector)
{
   auto deferred = selector->Activate();

   rpc::LuaDeferredPtr result = std::make_shared<rpc::LuaDeferredObject<XZRegionSelector>>(selector, "select xz region");

   deferred->Progress([L, result](csg::Cube3 const& c) {
      result->Notify(luabind::object(L, c));
   });
   deferred->Done([L, result](csg::Cube3 const& c) {
      result->Resolve(luabind::object(L, c));
   });
   deferred->Fail([L, result](std::string const& error) {
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
            return lua::ScriptHost::CoerseToBool(cb_(input));
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
   TraceRenderFramePromise() :
      frame_start_slot_("lua frame start"),
      server_tick_slot_("lua server tick")
   {
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

   TraceRenderFramePromisePtr OnFrameStart(std::string const& reason, luabind::object cb) {
      luabind::object callback(L_, cb);
      guards_ += frame_start_slot_.Register([=](FrameStartInfo const &info) {
         perfmon::TimelineCounterGuard tcg(reason.c_str());
         luabind::call_function<void>(callback, info.now, info.interpolate, info.frame_time, info.frame_time_wallclock);
      });
      return shared_from_this();
   }

   TraceRenderFramePromisePtr OnServerTick(std::string const& reason, luabind::object cb) {
      luabind::object callback(L_, cb);
      guards_ += server_tick_slot_.Register([=](int now) {
         perfmon::TimelineCounterGuard tcg(reason.c_str());
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

om::DataStorePtr Client_CreateDataStore(lua_State* L)
{
   // make sure we return the strong pointer version
   om::DataStorePtr db = Client_AllocObject<om::DataStore>();
   db->SetData(newtable(L));
   return db;
}

bool Client_IsValidStandingRegion(lua_State* L, csg::Region3 const& r)
{
   return Client::GetInstance().GetOctTree().GetNavGrid().IsStandable(r);
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
DEFINE_INVALID_LUA_CONVERSION(Input)
DEFINE_INVALID_LUA_CONVERSION(MouseInput)
DEFINE_INVALID_LUA_CONVERSION(KeyboardInput)
DEFINE_INVALID_LUA_CONVERSION(RawInput)
DEFINE_INVALID_LUA_CONVERSION(RaycastResult)

static bool Client_IsKeyDown(int key)
{
   return glfwGetKey(glfwGetCurrentContext(), key) == GLFW_PRESS;
}

static bool Client_IsMouseButtonDown(int button)
{
   return glfwGetMouseButton(glfwGetCurrentContext(), button) == GLFW_PRESS;
}

static csg::Point2 Client_GetMousePosition()
{
   return Client::GetInstance().GetMousePosition();
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
            def("get_object",                      &Client_GetObject),
            def("select_entity",                   &Client_SelectEntity),
            def("hilight_entity",                  &Client_HilightEntity),
            def("create_authoring_entity",         &Client_CreateAuthoringEntity),
            def("destroy_authoring_entity",        &Client_DestroyAuthoringEntity),
            def("create_render_entity",            &Client_CreateRenderEntity),
            def("create_render_entity",            &Client_CreateRenderEntityUnparented),
            def("get_render_entity",               &Client_GetRenderEntity),
            def("capture_input",                   &Client_CaptureInput),
            def("query_scene",                     &Client_QueryScene),
            def("trace_render_frame",              &Client_TraceRenderFrame),
            def("set_cursor",                      &Client_SetCursor),
            def("create_voxel_node",               &Client_CreateVoxelNode),
            def("create_obj_render_node",          &Client_CreateObjRenderNode),
            def("create_qubicle_matrix_node",      &Client_CreateQubicleMatrixNode),
            def("create_designation_node",         &Client_CreateDesignationNode),
            def("create_selection_node",           &Client_CreateSelectionNode),
            def("create_stockpile_node",           &Client_CreateStockpileNode),
            def("alloc_region",                    &Client_AllocObject<om::Region3Boxed>),
            def("alloc_region2",                   &Client_AllocObject<om::Region2Boxed>),
            def("create_datastore",                &Client_CreateDataStore),
            def("is_valid_standing_region",        &Client_IsValidStandingRegion),
            def("is_key_down",                     &Client_IsKeyDown),
            def("is_mouse_button_down",            &Client_IsMouseButtonDown),
            def("get_mouse_position",              &Client_GetMousePosition),

            def("create_xz_region_selector",       &Client_CreateXZRegionSelector),
            lua::RegisterTypePtr_NoTypeInfo<XZRegionSelector>("xz_region_selector")
                .def("require_supported",          &XZRegionSelector::RequireSupported)
                .def("require_unblocked",          &XZRegionSelector::RequireUnblocked)
                .def("activate",                   &Client_ActivateXZRegionSelector)
            ,

            lua::RegisterTypePtr_NoTypeInfo<CaptureInputPromise>("CaptureInputPromise")
               .def("on_input",          &CaptureInputPromise::OnInput)
               .def("destroy",           &CaptureInputPromise::Destroy)
            ,
            lua::RegisterTypePtr_NoTypeInfo<TraceRenderFramePromise>("TraceRenderFramePromise")
               .def("on_server_tick",    &TraceRenderFramePromise::OnServerTick)
               .def("on_frame_start",    &TraceRenderFramePromise::OnFrameStart)
               .def("destroy",           &TraceRenderFramePromise::Destroy)
            ,
            lua::RegisterTypePtr_NoTypeInfo<SetCursorPromise>("SetCursorPromise")
               .def("destroy",           &SetCursorPromise::Destroy)
            ,
            // xxx: Input, MouseInput, KeyboardInput, etc. should be in open_core.cpp, right?
            lua::RegisterType_NoTypeInfo<Input>("Input")
               .enum_("constants") [
                  value("MOUSE", Input::MOUSE),
                  value("KEYBOARD", Input::KEYBOARD),
                  value("RAW_INPUT", Input::RAW_INPUT)
               ]
               .def_readonly("type",      &Input::type)
               .def_readonly("mouse",     &Input::mouse)
               .def_readonly("keyboard",  &Input::keyboard)
               .def_readonly("raw_input", &Input::raw_input)
               .def_readonly("focused",   &Input::focused)
            ,
            lua::RegisterType_NoTypeInfo<MouseInput>("MouseInput")
               .def_readonly("x",       &MouseInput::x)
               .def_readonly("y",       &MouseInput::y)
               .def_readonly("dx",      &MouseInput::dx)
               .def_readonly("dy",      &MouseInput::dy)
               .def_readonly("wheel",   &MouseInput::wheel)
               .def_readonly("in_client_area", &MouseInput::in_client_area)
               .def("up",               &MouseEvent_GetUp)
               .def("down",             &MouseEvent_GetDown)
               .def_readonly("dragging",&MouseInput::dragging)
               .enum_("constants") [
                  value("MOUSE_BUTTON_1",    GLFW_MOUSE_BUTTON_1),
                  value("MOUSE_BUTTON_2",    GLFW_MOUSE_BUTTON_2),
                  value("MOUSE_BUTTON_3",    GLFW_MOUSE_BUTTON_3),
                  value("MOUSE_BUTTON_4",    GLFW_MOUSE_BUTTON_4)
               ]
            ,
            lua::RegisterType_NoTypeInfo<KeyboardInput>("KeyboardInput")
               .enum_("constants") [
                  
                  value("KEY_SPACE",         GLFW_KEY_SPACE),
                  value("KEY_APOSTROPHE",    GLFW_KEY_APOSTROPHE),
                  value("KEY_COMMA",         GLFW_KEY_COMMA),
                  value("KEY_MINUS",         GLFW_KEY_MINUS),
                  value("KEY_PERIOD",        GLFW_KEY_PERIOD),
                  value("KEY_SLASH",         GLFW_KEY_SLASH),
                  value("KEY_0",             GLFW_KEY_0),
                  value("KEY_1",             GLFW_KEY_1),
                  value("KEY_2",             GLFW_KEY_2),
                  value("KEY_3",             GLFW_KEY_3),
                  value("KEY_4",             GLFW_KEY_4),
                  value("KEY_5",             GLFW_KEY_5),
                  value("KEY_6",             GLFW_KEY_6),
                  value("KEY_7",             GLFW_KEY_7),
                  value("KEY_8",             GLFW_KEY_8),
                  value("KEY_9",             GLFW_KEY_9),
                  value("KEY_SEMICOLON",     GLFW_KEY_SEMICOLON),
                  value("KEY_EQUAL",         GLFW_KEY_EQUAL),
                  value("KEY_A",             GLFW_KEY_A),
                  value("KEY_B",             GLFW_KEY_B),
                  value("KEY_C",             GLFW_KEY_C),
                  value("KEY_D",             GLFW_KEY_D),
                  value("KEY_E",             GLFW_KEY_E),
                  value("KEY_F",             GLFW_KEY_F),
                  value("KEY_G",             GLFW_KEY_G),
                  value("KEY_H",             GLFW_KEY_H),
                  value("KEY_I",             GLFW_KEY_I),
                  value("KEY_J",             GLFW_KEY_J),
                  value("KEY_K",             GLFW_KEY_K),
                  value("KEY_L",             GLFW_KEY_L),
                  value("KEY_M",             GLFW_KEY_M),
                  value("KEY_N",             GLFW_KEY_N),
                  value("KEY_O",             GLFW_KEY_O),
                  value("KEY_P",             GLFW_KEY_P),
                  value("KEY_Q",             GLFW_KEY_Q),
                  value("KEY_R",             GLFW_KEY_R),
                  value("KEY_S",             GLFW_KEY_S),
                  value("KEY_T",             GLFW_KEY_T),
                  value("KEY_U",             GLFW_KEY_U),
                  value("KEY_V",             GLFW_KEY_V),
                  value("KEY_W",             GLFW_KEY_W),
                  value("KEY_X",             GLFW_KEY_X),
                  value("KEY_Y",             GLFW_KEY_Y),
                  value("KEY_Z",             GLFW_KEY_Z),
                  value("KEY_LEFT_BRACKET",  GLFW_KEY_LEFT_BRACKET),
                  value("KEY_BACKSLASH",     GLFW_KEY_BACKSLASH),
                  value("KEY_RIGHT_BRACKET", GLFW_KEY_RIGHT_BRACKET),
                  value("KEY_GRAVE_ACCENT",  GLFW_KEY_GRAVE_ACCENT),
                  value("KEY_WORLD_1",       GLFW_KEY_WORLD_1),
                  value("KEY_WORLD_2",       GLFW_KEY_WORLD_2),
                  value("KEY_ESC",        GLFW_KEY_ESCAPE),
                  value("KEY_ENTER",         GLFW_KEY_ENTER),
                  value("KEY_TAB",           GLFW_KEY_TAB),
                  value("KEY_BACKSPACE",     GLFW_KEY_BACKSPACE),
                  value("KEY_INSERT",        GLFW_KEY_INSERT),
                  value("KEY_DELETE",        GLFW_KEY_DELETE),
                  value("KEY_RIGHT",         GLFW_KEY_RIGHT),
                  value("KEY_LEFT",          GLFW_KEY_LEFT),
                  value("KEY_DOWN",          GLFW_KEY_DOWN),
                  value("KEY_UP",            GLFW_KEY_UP),
                  value("KEY_PAGE_UP",       GLFW_KEY_PAGE_UP),
                  value("KEY_PAGE_DOWN",     GLFW_KEY_PAGE_DOWN),
                  value("KEY_HOME",          GLFW_KEY_HOME),
                  value("KEY_END",           GLFW_KEY_END),
                  value("KEY_CAPS_LOCK",     GLFW_KEY_CAPS_LOCK),
                  value("KEY_SCROLL_LOCK",   GLFW_KEY_SCROLL_LOCK),
                  value("KEY_NUM_LOCK",      GLFW_KEY_NUM_LOCK),
                  value("KEY_PRINT_SCREEN",  GLFW_KEY_PRINT_SCREEN),
                  value("KEY_PAUSE",         GLFW_KEY_PAUSE),
                  value("KEY_F1",            GLFW_KEY_F1),
                  value("KEY_F2",            GLFW_KEY_F2),
                  value("KEY_F3",            GLFW_KEY_F3),
                  value("KEY_F4",            GLFW_KEY_F4),
                  value("KEY_F5",            GLFW_KEY_F5),
                  value("KEY_F6",            GLFW_KEY_F6),
                  value("KEY_F7",            GLFW_KEY_F7),
                  value("KEY_F8",            GLFW_KEY_F8),
                  value("KEY_F9",            GLFW_KEY_F9),
                  value("KEY_F10",           GLFW_KEY_F10),
                  value("KEY_F11",           GLFW_KEY_F11),
                  value("KEY_F12",           GLFW_KEY_F12),
                  value("KEY_KP_0",          GLFW_KEY_KP_0),
                  value("KEY_KP_1",          GLFW_KEY_KP_1),
                  value("KEY_KP_2",          GLFW_KEY_KP_2),
                  value("KEY_KP_3",          GLFW_KEY_KP_3),
                  value("KEY_KP_4",          GLFW_KEY_KP_4),
                  value("KEY_KP_5",          GLFW_KEY_KP_5),
                  value("KEY_KP_6",          GLFW_KEY_KP_6),
                  value("KEY_KP_7",          GLFW_KEY_KP_7),
                  value("KEY_KP_8",          GLFW_KEY_KP_8),
                  value("KEY_KP_9",          GLFW_KEY_KP_9),
                  value("KEY_KP_DECIMAL",    GLFW_KEY_KP_DECIMAL),
                  value("KEY_KP_DIVIDE",     GLFW_KEY_KP_DIVIDE),
                  value("KEY_KP_MULTIPLY",   GLFW_KEY_KP_MULTIPLY),
                  value("KEY_KP_SUBTRACT",   GLFW_KEY_KP_SUBTRACT),
                  value("KEY_KP_ADD",        GLFW_KEY_KP_ADD),
                  value("KEY_KP_ENTER",      GLFW_KEY_KP_ENTER),
                  value("KEY_KP_EQUAL",      GLFW_KEY_KP_EQUAL),
                  value("KEY_LEFT_SHIFT",    GLFW_KEY_LEFT_SHIFT),
                  value("KEY_LEFT_CONTROL",  GLFW_KEY_LEFT_CONTROL),
                  value("KEY_LEFT_ALT",      GLFW_KEY_LEFT_ALT),
                  value("KEY_LEFT_SUPER",    GLFW_KEY_LEFT_SUPER),
                  value("KEY_RIGHT_SHIFT",   GLFW_KEY_RIGHT_SHIFT),
                  value("KEY_RIGHT_CONTROL", GLFW_KEY_RIGHT_CONTROL),
                  value("KEY_RIGHT_ALT",     GLFW_KEY_RIGHT_ALT),
                  value("KEY_RIGHT_SUPER",   GLFW_KEY_RIGHT_SUPER),
                  value("KEY_MENU",          GLFW_KEY_MENU)
               ]
               .def_readonly("key",    &KeyboardInput::key)
               .def_readonly("down",   &KeyboardInput::down)
            ,
            lua::RegisterType_NoTypeInfo<RawInput>("RawInput")
         ]
      ]
   ];
}
