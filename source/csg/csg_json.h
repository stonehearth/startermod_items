#ifndef _RADIANT_CSG_JSON_H
#define _RADIANT_CSG_JSON_H

#include "radiant_json.h"

namespace radiant {
   namespace json {
      template <> static csg::Point3 cast(ConstJsonObject const & node, csg::Point3 const& def) {
         if (node.type() != JSON_ARRAY || node.size() != 3) {
            return def;
         }

         JSONNode const& n = node.GetNode();
         csg::Point3 result;
         for (int i = 0; i < 3; i++) {
            if (n.at(i).type() != JSON_NUMBER) {
               return def;
            }
            result[i] = n.at(i).as_int();
         }
         return result;
      }

      template <> static csg::Cube3 cast(ConstJsonObject const & node, csg::Cube3 const& def) {
         return csg::Cube3(node.get<csg::Point3>(0),
                           node.get<csg::Point3>(1));
      }

      template <> static csg::Region3 cast(ConstJsonObject const & node, csg::Region3 const& def) {
         csg::Region3 result;
         for (auto const& entry : node) {
            result += ConstJsonObject(entry).as<csg::Cube3>();
         }
         return result;
      }
   };
};


#endif // _RADIANT_CSG_JSON_H
