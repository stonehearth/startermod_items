#include "dm/dm_save_impl.h"
#include "csg/region.h"
#include "csg/color.h"
#include "csg/transform.h"
#include "csg/sphere.h"
#include "lib/json/node.h"
#include "lib/lua/controller_object.h"
#include "lib/lua/data_object.h"
#include "om/selection.h"
#include "store.h"

IMPLEMENT_DM_BASIC_TYPE(int,  Protocol::integer);
IMPLEMENT_DM_BASIC_TYPE(bool, Protocol::boolean);
IMPLEMENT_DM_BASIC_TYPE(float, Protocol::floatingpoint);
IMPLEMENT_DM_BASIC_TYPE(std::string, Protocol::string);
IMPLEMENT_DM_EXTENSION(csg::Color3, Protocol::color)
IMPLEMENT_DM_EXTENSION(csg::Color4, Protocol::color)
IMPLEMENT_DM_EXTENSION(csg::Cube3, Protocol::cube3i)
IMPLEMENT_DM_EXTENSION(csg::Cube3f, Protocol::cube3f)
IMPLEMENT_DM_EXTENSION(csg::Point2, Protocol::point2i)
IMPLEMENT_DM_EXTENSION(csg::Point3, Protocol::point3i)
IMPLEMENT_DM_EXTENSION(csg::Point3f, Protocol::point3f)
IMPLEMENT_DM_EXTENSION(csg::Region3, Protocol::region3i)
IMPLEMENT_DM_EXTENSION(csg::Region2, Protocol::region2i)
IMPLEMENT_DM_EXTENSION(csg::Sphere, Protocol::sphere3f)
IMPLEMENT_DM_EXTENSION(csg::Transform, Protocol::transform)
IMPLEMENT_DM_EXTENSION(om::Selection, Protocol::Selection::extension)
IMPLEMENT_DM_NOP(radiant::lua::ControllerObject);

template<>
struct radiant::dm::SaveImpl<radiant::json::Node>
{
   static void SaveValue(const Store& store, Protocol::Value* msg, const radiant::json::Node& node) {
      // A compile error here probably means you do not have the corrent
      // template specialization for your type.  See IMPLEMENT_DM_EXTENSION
      // below.
      std::string txt = node.write();
      ASSERT(!txt.empty());
      ASSERT(libjson::is_valid(txt));
      msg->SetExtension(Protocol::string, node.write());
   }
   static void LoadValue(const Store& store, const Protocol::Value& msg, radiant::json::Node& node) {
      std::string txt = msg.GetExtension(Protocol::string);
      ASSERT(!txt.empty());
      ASSERT(libjson::is_valid(txt));
      node = radiant::json::Node(libjson::parse(txt));
   }
   static void GetDbgInfo(radiant::json::Node const& node, DbgInfo &info) {
      info.os << "... json ...";
   }
};

template<>
struct dm::SaveImpl<lua::DataObject> {
   static void SaveValue(const dm::Store& store, Protocol::Value* msg, lua::DataObject const& obj) {
      SaveImpl<json::Node>::SaveValue(store, msg, obj.GetJsonNode());
   }
   static void LoadValue(const Store& store, const Protocol::Value& msg, lua::DataObject& obj) {
      json::Node node;
      SaveImpl<json::Node>::LoadValue(store, msg, node);
      obj.SetJsonNode(store.GetInterpreter(), node);
   }
   static void GetDbgInfo(lua::DataObject const& obj, dm::DbgInfo &info) {
      info.os << "[data_object " << obj.GetJsonNode().write() << "]";
   }
};
