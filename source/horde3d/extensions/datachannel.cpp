
#include "datachannel.h"

#include "libjson.h"
#include "namespace.h"
#include "radiant_json.h"

using namespace ::radiant;
using namespace ::radiant::json;
using namespace ::radiant::horde3d;

Vec4f parseVec4f(ConstJsonObject &vals, const Vec4f& def)
{
   return Vec4f(vals.get(0, def.x), vals.get(1, def.y), vals.get(2, def.z), vals.get(3, def.w));
}

Vec3f parseVec3f(ConstJsonObject &vals, const Vec3f& def)
{
   return Vec3f(vals.get(0, def.x), vals.get(1, def.y), vals.get(2, def.z));
}

std::vector<std::pair<float, float> > parseCurveValues(ConstJsonObject &n) {
   std::vector<std::pair<float, float> > result;

   for (const auto &child : n)
   {
      result.push_back(std::pair<float, float>((float)child.at(0).as_float(), (float)child.at(1).as_float()));
   }

   return result;
}

ValueEmitter<float>* ::radiant::horde3d::parseChannel(ConstJsonObject &n, const char *childName, float def)
{
   if (!n.has(childName))
   {
      return new ConstantValueEmitter<float>(def);
   }

   auto childNode = n.getn(childName);

   if (!childNode.has("values") || !childNode.has("kind"))
   {
      return new ConstantValueEmitter<float>(def);
   }
   auto vals = childNode.getn("values");
   std::string kind = childNode.get<std::string>("kind");

   if (kind == "CONSTANT")
   {
      return new ConstantValueEmitter<float>(vals.get(0, def));
   } else if (kind == "RANDOM_BETWEEN")
   {
      return new RandomBetweenValueEmitter(vals.get(0, def), vals.get(1, def));
   } else if (kind == "CURVE")
   {
      return new LinearCurveValueEmitter(parseCurveValues(vals));
   } //else kind == "RANDOM_BETWEEN_CURVES"

   return new RandomBetweenLinearCurvesValueEmitter(
      parseCurveValues(vals.getn(0)), parseCurveValues(vals.getn(1)));
}

ValueEmitter<Vec3f>* ::radiant::horde3d::parseChannel(ConstJsonObject &n, const char *childName, const Vec3f &def)
{
   if (!n.has(childName))
   {
      return new ConstantValueEmitter<Vec3f>(def);
   }

   auto childNode = n.getn(childName);

   if (!childNode.has("values") || !childNode.has("kind"))
   {
      return new ConstantValueEmitter<Vec3f>(def);
   }
   auto vals = childNode.getn("values");
   std::string kind = childNode.get<std::string>("kind");

   if (kind == "CONSTANT")
   {
      Vec3f val(parseVec3f(vals, def));
      return new ConstantValueEmitter<Vec3f>(val);
   } //else kind == "RANDOM_BETWEEN"

   ConstJsonObject vec1node = vals.getn(0);
   ConstJsonObject vec2node = vals.getn(1);

   return new RandomBetweenVec3fEmitter(parseVec3f(vec1node, def), parseVec3f(vec2node, def));
}

ValueEmitter<Vec4f>* ::radiant::horde3d::parseChannel(ConstJsonObject &n, const char *childName, const Vec4f &def)
{
   if (!n.has(childName))
   {
      return new ConstantValueEmitter<Vec4f>(def);
   }

   auto childNode = n.getn(childName);

   if (!childNode.has("values") || !childNode.has("kind"))
   {
      return new ConstantValueEmitter<Vec4f>(def);
   }
   auto vals = childNode.getn("values");
   std::string kind = childNode.get<std::string>("kind");

   if (kind == "CONSTANT")
   {
      return new ConstantValueEmitter<Vec4f>(parseVec4f(vals, def));
   } //else kind == "RANDOM_BETWEEN"

   return new RandomBetweenVec4fEmitter(parseVec4f(vals.getn(0), def), 
      parseVec4f(vals.getn(1), def));
}
