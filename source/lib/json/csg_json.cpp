#include "radiant.h"
#include "csg/point.h"
#include "csg/cube.h"
#include "csg/region.h"
#include "csg/quaternion.h"
#include "csg/transform.h"
#include "csg/ray.h"
#include "csg/color.h"
#include "csg/util.h" // xxx: should be csg/edge.h
#include "csg/heightmap.h"
#include "csg/random_number_generator.h"
#include "lib/json/node.h"
#include "lib/json/macros.h"
#include "lib/json/csg_json.h"

using namespace radiant;
using namespace radiant::json;

template <> csg::Transform json::decode(Node const& node) {
   csg::Transform value;
   value.position = node.get<csg::Point3f>("position");
   value.orientation = node.get<csg::Quaternion>("orientation");
   return value;
}

template <> Node json::encode(csg::Transform const& value) {
   Node node;
   node.set("position", value.position);
   node.set("orientation", value.orientation);
   return node;
}

template<> csg::Quaternion json::decode(Node const& node) {
   csg::Quaternion value;
   char coord = 'w';
   for (int i = 0; i < 4; i++) {
      std::string c(1, coord + i);
      if (!node.has(c)) {
         throw std::invalid_argument("malformated Quaternion in json");
      }
      value[i] = node.get<float>(c);
   }
   return value;
}

template<> Node json::encode(csg::Quaternion const& value) {
   Node node;
   char coord = 'w';
   for (int i = 0; i < 4; i++) {
      node.set(std::string(1, coord + i), value[i]);
   }
   return node;
}

template <> csg::Ray3 json::decode(Node const& node) {
   csg::Ray3 value;
   value.origin = node.get<csg::Point3f>("origin");
   value.direction = node.get<csg::Point3f>("direction");
   return value;
}

template <> Node json::encode(csg::Ray3 const& value) {
   Node node;
   node.set("origin", value.origin);
   node.set("direction", value.direction);
   return node;
}

#define DECLARE_TYPE(lower, T) \
   template <> T json::decode(Node const& node) { \
      return decode_ ## lower<T>(node); \
   } \
   template <> Node json::encode(T const& node) { \
      return encode_ ## lower<T>(node); \
   }

template <typename T>
static T decode_point(Node const& node) {
   T value;
   char coord = 'x';
   for (int i = 0; i < T::Dimension; i++) {
      std::string c(1, coord + i);
      if (!node.has(c)) {
         throw std::invalid_argument("malformated Point in json");
      }
      value[i] = node.get<typename T::Scalar>(c);
   }
   return value;
}

template <typename T>
static Node encode_point(T const& value) {
   Node node;
   char coord = 'x';
   for (int i = 0; i < T::Dimension; i++) {
      node.set(std::string(1, coord + i), value[i]);
   }
   return node;
}

template <typename T>
static T decode_cube(Node const& node) {
   T value;
   value.min = node.get<typename T::Point>("min");
   value.max = node.get<typename T::Point>("max");
   return value;
}

template <typename T>
static Node encode_cube(T const& value) {
   Node node;
   node.set("min", value.min);
   node.set("max", value.max);
   return node;
}

template <typename T>
static T decode_region(Node const& node) {
   T value;
   for (Node const& n : node) {
      value.AddUnique(n.as<typename T::Cube>());
   }
   return value;
}

template <typename T>
static Node encode_region(T const& value) {
   Node node(JSONNode(JSON_ARRAY));
   for (auto const& cube : value) {
      node.add(cube);
   }
   return node;
}

template <typename T>
static T decode_color(Node const& node) {
   return T::FromString(node.as<std::string>());
}

template <typename T>
static Node encode_color(T const& value) {
   return Node(JSONNode("", value.ToString()));
}

DECLARE_TYPE(point, csg::Point1)
DECLARE_TYPE(point, csg::Point2)
DECLARE_TYPE(point, csg::Point3)
DECLARE_TYPE(point, csg::Point1f)
DECLARE_TYPE(point, csg::Point2f)
DECLARE_TYPE(point, csg::Point3f)
DECLARE_TYPE(cube,  csg::Line1)
DECLARE_TYPE(cube, csg::Rect2)
DECLARE_TYPE(cube, csg::Rect2f)
DECLARE_TYPE(cube, csg::Cube3)
DECLARE_TYPE(cube, csg::Cube3f)
DECLARE_TYPE(region, csg::Region1);
DECLARE_TYPE(region, csg::Region2);
DECLARE_TYPE(region, csg::Region2f);
DECLARE_TYPE(region, csg::Region3);
DECLARE_TYPE(region, csg::Region3f);
DECLARE_TYPE(color, csg::Color3);
DECLARE_TYPE(color, csg::Color4);

DEFINE_INVALID_JSON_CONVERSION(std::shared_ptr<csg::HeightMap<double>>)
DEFINE_INVALID_JSON_CONVERSION(std::shared_ptr<csg::EdgePointX>)
DEFINE_INVALID_JSON_CONVERSION(std::shared_ptr<csg::EdgeX>)
DEFINE_INVALID_JSON_CONVERSION(std::shared_ptr<csg::EdgeList>)
DEFINE_INVALID_JSON_CONVERSION(csg::RandomNumberGenerator)
