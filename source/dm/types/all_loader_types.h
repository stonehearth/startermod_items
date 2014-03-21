#include "dm/dm_save_impl.h"
#include "csg/region.h"
#include "csg/color.h"
#include "csg/transform.h"
#include "csg/sphere.h"
#include "csg/ray.h"
#include "lib/json/node.h"
#include "lib/lua/controller_object.h"
#include "lib/lua/data_object.h"
#include "om/selection.h"
#include "protocols/store.pb.h"
#include "dm/store.h"
#include "lib/marshall/convert.h"

IMPLEMENT_DM_BASIC_TYPE(int,  Protocol::integer);
IMPLEMENT_DM_BASIC_TYPE(bool, Protocol::boolean);
IMPLEMENT_DM_BASIC_TYPE(float, Protocol::floatingpoint);
IMPLEMENT_DM_BASIC_TYPE(std::string, Protocol::string);
IMPLEMENT_DM_EXTENSION(csg::Color3, Protocol::color)
IMPLEMENT_DM_EXTENSION(csg::Color4, Protocol::color)
IMPLEMENT_DM_EXTENSION(csg::Cube3, Protocol::cube3i)
IMPLEMENT_DM_EXTENSION(csg::Cube3f, Protocol::cube3f)
IMPLEMENT_DM_EXTENSION(csg::Line1, Protocol::cube1i)
IMPLEMENT_DM_EXTENSION(csg::Rect2, Protocol::cube2i)
IMPLEMENT_DM_EXTENSION(csg::Rect2f, Protocol::cube2f)
IMPLEMENT_DM_EXTENSION(csg::Point1, Protocol::point1i)
IMPLEMENT_DM_EXTENSION(csg::Point1f, Protocol::point1f)
IMPLEMENT_DM_EXTENSION(csg::Point2, Protocol::point2i)
IMPLEMENT_DM_EXTENSION(csg::Point2f, Protocol::point2f)
IMPLEMENT_DM_EXTENSION(csg::Point3, Protocol::point3i)
IMPLEMENT_DM_EXTENSION(csg::Point3f, Protocol::point3f)
IMPLEMENT_DM_EXTENSION(csg::Region3, Protocol::region3i)
IMPLEMENT_DM_EXTENSION(csg::Region3f, Protocol::region3f)
IMPLEMENT_DM_EXTENSION(csg::Region2, Protocol::region2i)
IMPLEMENT_DM_EXTENSION(csg::Region2f, Protocol::region2f)
IMPLEMENT_DM_EXTENSION(csg::Region1, Protocol::region1i)
IMPLEMENT_DM_EXTENSION(csg::Region1f, Protocol::region1f)
IMPLEMENT_DM_EXTENSION(csg::Sphere, Protocol::sphere3f)
IMPLEMENT_DM_EXTENSION(csg::Transform, Protocol::transform)
IMPLEMENT_DM_EXTENSION(csg::Quaternion, Protocol::quaternion)
IMPLEMENT_DM_EXTENSION(csg::Ray3, Protocol::ray3f)
IMPLEMENT_DM_EXTENSION(om::Selection, Protocol::Selection::extension)
IMPLEMENT_DM_EXTENSION(lua::ControllerObject, Protocol::LuaControllerObject::extension)


template<>
struct dm::SaveImpl<lua::DataObject> {
   static void SaveValue(const dm::Store& store, dm::SerializationType r, Protocol::Value* msg, lua::DataObject const& obj) {
      obj.SaveValue(store, r, msg->MutableExtension(Protocol::LuaDataObject::extension));
   }
   static void LoadValue(const Store& store, dm::SerializationType r, const Protocol::Value& msg, lua::DataObject& obj) {
      obj.LoadValue(store, r, msg.GetExtension(Protocol::LuaDataObject::extension));
   }
   static void GetDbgInfo(lua::DataObject const& obj, dm::DbgInfo &info) {
      info.os << "[data_object]";
   }
};

template<>
struct dm::SaveImpl<luabind::object> {
   static void SaveValue(const dm::Store& store, dm::SerializationType r, Protocol::Value* msg, luabind::object const& obj) {
      int flags;
      if (r == REMOTING) { flags |= marshall::Convert::REMOTE; }
      marshall::Convert(store, flags).ToProtobuf(obj, msg);
   }
   static void LoadValue(const Store& store, dm::SerializationType r, const Protocol::Value& msg, luabind::object& obj) {
      int flags;
      if (r == REMOTING) { flags |= marshall::Convert::REMOTE; }
      marshall::Convert(store, flags).ToLua(msg, obj);
   }
   static void GetDbgInfo(lua::DataObject const& obj, dm::DbgInfo &info) {
      info.os << "[lua_object]";
   }
};


template<>
struct radiant::dm::SaveImpl<radiant::json::Node>
{
   static void SaveValue(const Store& store, SerializationType r, Protocol::Value* msg, const radiant::json::Node& node) {
      // A compile error here probably means you do not have the corrent
      // template specialization for your type.  See IMPLEMENT_DM_EXTENSION
      // below.
      std::string txt = node.write();
      ASSERT(!txt.empty());
      ASSERT(libjson::is_valid(txt));
      msg->SetExtension(Protocol::string, node.write());
   }
   static void LoadValue(const Store& store, SerializationType r, const Protocol::Value& msg, radiant::json::Node& node) {
      std::string txt = msg.GetExtension(Protocol::string);
      ASSERT(!txt.empty());
      ASSERT(libjson::is_valid(txt));
      node = radiant::json::Node(libjson::parse(txt));
   }
   static void GetDbgInfo(radiant::json::Node const& node, DbgInfo &info) {
      info.os << "... json ...";
   }
};
