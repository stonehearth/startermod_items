#ifndef _RADIANT_CSG_JSON_H
#define _RADIANT_CSG_JSON_H

#include "radiant_json.h"

namespace radiant {
   namespace json {
      template <> static csg::Point3 cast_(JSONNode const& node) {
         int x(cast_<int>(node.at(0)));
         int y(cast_<int>(node.at(1)));
         int z(cast_<int>(node.at(2)));
         return csg::Point3(x, y, z);
      }

      template <> static csg::Cube3 cast_(JSONNode const& node) {
         csg::Point3 min(cast_<csg::Point3>(node.at(0)));
         csg::Point3 max(cast_<csg::Point3>(node.at(1)));
         return csg::Cube3(min, max);
      }

      template <> static csg::Region3 cast_(JSONNode const& node) {
         csg::Region3 result;
         for (auto const& entry : node) {
            result += cast_<csg::Cube3>(entry);
         }
         return result;
      }
   };
};


#endif // _RADIANT_CSG_JSON_H
