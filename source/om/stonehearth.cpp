#include "pch.h"
#include "stonehearth.h"
#include "entity.h"
#include "radiant_json.h"
#include "resources/res_manager.h"

using namespace ::radiant;
using namespace ::radiant::om;

csg::Region3 Stonehearth::ComputeStandingRegion(const csg::Region3& r, int height)
{
   csg::Region3 standing;
   for (const csg::Cube3& c : r) {
      auto p0 = c.GetMin();
      auto p1 = c.GetMax();
      auto w = p1[0] - p0[0], h = p1[2] - p0[2];
      standing.Add(csg::Cube3(p0 - csg::Point3(0, 0, 1), p0 + csg::Point3(w, height, 0)));  // top
      standing.Add(csg::Cube3(p0 - csg::Point3(1, 0, 0), p0 + csg::Point3(0, height, h)));  // left
      standing.Add(csg::Cube3(p0 + csg::Point3(0, 0, h), p0 + csg::Point3(w, height, h + 1)));  // bottom
      standing.Add(csg::Cube3(p0 + csg::Point3(w, 0, 0), p0 + csg::Point3(w + 1, height, h)));  // right
   }
   standing.Optimize();

   return standing;
}


// actualyl, don't die.  just be like "this is the crappy version:
om::EntityPtr Stonehearth::CreateEntityLegacyDIEDIEDIE(dm::Store& store, std::string uri)
{
   om::EntityPtr entity = store.AllocObject<om::Entity>();   

   JSONNode const& node = resources::ResourceManager2::GetInstance().LookupJson(uri);
   auto i = node.find("components");
   if (i != node.end() && i->type() == JSON_NODE) {
      for (auto const& entry : *i) {
   #define OM_OBJECT(Cls, lower) \
         if (entry.name() == #lower) { \
            auto component = entity->AddComponent<om::Cls>(); \
            component->ExtendObject(json::ConstJsonObject(entry)); \
            continue; \
         }
         OM_ALL_COMPONENTS
   #undef OM_OBJECT
      }
   }
   return entity;
}
