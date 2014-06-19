#ifndef _RADIANT_PROTOCOL_PROTOCOL_H
#define _RADIANT_PROTOCOL_PROTOCOL_H

namespace radiant {
   namespace tesseract {
      namespace protocol {
         class Update;
         class AllocObjects;
      }
   }
   namespace protocol {
      class point4i;
      class point4f;
      class point3i;
      class point3f;
      class point2i;
      class point2f;
      class point1i;
      class point1f;
      class cube3i;
      class cube3f;
      class cube2i;
      class cube2f;
      class cube1i;
      class cube1f;
      class region3i;
      class region3f;
      class region2i;
      class region2f;
      class sphere3f;
      class ray3f;
      class quaternion;
      class transform;
      class color;
      class line;
      class triangle;
      class quad;
      class box;
      class coord;
      class shapelist;
      class cursor_description;
   };
};

// xxx:move this into radiant...
namespace Protocol {
   class Value;
   class Object;
   class Store;
   class LuaObject;
   class LuaObject_Table;
   class LuaControllerObject;
   class LuaDataObject;
}

#endif  // _RADIANT_PROTOCOL_PROTOCOL_H
