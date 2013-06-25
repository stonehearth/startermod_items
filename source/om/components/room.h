#ifndef _RADIANT_OM_ROOM_H
#define _RADIANT_OM_ROOM_H

#include "math3d.h"
#include "om/om.h"
#include "om/all_object_types.h"
#include "dm/boxed.h"
#include "dm/record.h"
#include "dm/object.h"
#include "dm/store.h"
#include "dm/array.h"
#include "namespace.h"
#include "component.h"
#include "csg/region.h"
#include "build_orders.h"

BEGIN_RADIANT_OM_NAMESPACE

class Room : public Component

{
public:
   DEFINE_OM_OBJECT_TYPE(Room, room);

   void SetBounds(const math3d::ibounds3& bounds) {
      SetInteriorSize(bounds._min, bounds._max);
   }
   void SetInteriorSize(const csg::Point3& p0, const csg::Point3& p1);   

   void StartProject(const dm::CloneMapping& mapping, std::function<void(BuildOrderPtr)> reg);

private:
   void InitializeRecordFields() override;
   EntityPtr CreatePost();
   EntityPtr CreateWall(EntityPtr post1, EntityPtr post2, const csg::Point3& normal);
   EntityPtr ClonePost();
   EntityPtr CloneWall();
   void Create();
   void CreateRoof();
   void CreateFloor();
   void ResizeRoom(const csg::Point3& size);

private:
   enum {
      TOP_LEFT = 0,
      TOP_RIGHT = 1,
      BOTTOM_LEFT = 2,
      BOTTOM_RIGHT = 3,
      TOP = 0,
      LEFT = 1,
      RIGHT = 2,
      BOTTOM = 3
   };
   dm::Boxed<csg::Point3>     dimensions_;
   dm::Boxed<EntityPtr>       floor_;
   dm::Boxed<EntityPtr>       roof_;
   dm::Array<EntityPtr, 4>    walls_;
   dm::Array<EntityPtr, 4>    posts_;
   dm::Set<EntityPtr>         scaffolding_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_ROOM_H
