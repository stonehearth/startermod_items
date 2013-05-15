#include "pch.h"
#include "room.h"
#include "post.h"
#include "wall.h"
#include "mob.h"
#include "floor.h"
#include "peaked_roof.h"
#include "om/stonehearth.h"
#include "build_orders.h"
#include "entity_container.h"
#include "component_helpers.h"
#include "build_order_dependencies.h"

using namespace ::radiant;
using namespace ::radiant::om;

static const int RoomHeight = 6;


Room::Room()
{
}

Room::~Room()
{
}

void Room::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("dimensions", dimensions_);
   AddRecordField("walls", walls_);
   AddRecordField("posts", posts_);
   AddRecordField("floor", floor_);
   AddRecordField("roof", roof_);
   AddRecordField("scaffolding", scaffolding_);
}

void Room::StartProject(const dm::CloneMapping& mapping, std::function<void(BuildOrderPtr)> regcb)
{
   auto roof = (*roof_)->GetComponent<PeakedRoof>();
   auto floor =  (*floor_)->GetComponent<Floor>();

   roof->StartProject(mapping);
   auto roofDependencies = roof->GetEntity().AddComponent<BuildOrderDependencies>();
   floor->StartProject(mapping);

   for (int i = 0; i < 4; i++) {
      auto post = posts_[i]->GetComponent<Post>();
      post->StartProject(mapping);
   }

   bool first = true;
   for (int i = 0; i < 4; i++) {
      auto wall = walls_[i]->GetComponent<Wall>();

      wall->StartProject(mapping);

      // If we let all the walls go up before the floor is finished, we may easily
      // get to a point where workers can run into a room to build the floor, but
      // can't get out!  The scaffolding is blocking all the doorways and you can
      // currently fall farther than you're allowed to climb...
      auto dep = wall->GetEntity().AddComponent<BuildOrderDependencies>();
      dep->AddDependency(floor->GetEntityRef());

      // Make the roof dependant on all the walls to prevent oddities like building
      // the overhang of a peaked roof before the wall supporting it is actually
      // finished
      roofDependencies->AddDependency(wall->GetEntityRef());

      // Add some scaffolding to support this wall...
      EntityPtr e = Stonehearth::CreateEntity(GetStore(), "module://stonehearth/buildings/scaffolding");
      GetEntity().GetComponent<EntityContainer>()->AddChild(e);
      scaffolding_.Insert(e);


      auto scaffolding = e->AddComponent<Scaffolding>();
      scaffolding->StartProject(wall, mapping);
      regcb(scaffolding);

      if (first) {
         first = false;
         scaffolding->SetRoof(roof);
      }
   }

}


void Room::SetInteriorSize(const csg::Point3& p0, const csg::Point3& p1)
{
   csg::Point3 size = csg::Cube3(p0, p1).GetSize();

   if (size[0] < 2 || size[2] < 2) {
      // Empty room!
      for (int i = 0; i < 4; i++) {
         walls_[i] = nullptr;
         posts_[i] = nullptr;
      }
      dimensions_.Modify() = csg::Point3(0, 0, 0);
      return;
   }

   LOG(WARNING) << "Moving room " << GetEntity().GetObjectId() << " to " << p0;
   auto mob = GetEntity().AddComponent<Mob>();
   mob->SetInterpolateMovement(false);
   mob->MoveToGridAligned(p0);

   if (!posts_[0]) {
      Create();
   }
   size[1] = RoomHeight;
   ResizeRoom(size);
}

EntityPtr Room::CreatePost()
{
   om::EntityPtr entity = GetStore().AllocObject<Entity>();

   entity->SetDebugName("post for room " + stdutil::ToString(GetEntity().GetEntityId()));
   auto mob = entity->AddComponent<Mob>();
   mob->SetInterpolateMovement(false);

   entity->AddComponent<Post>()->Create(RoomHeight);

   GetEntity().AddComponent<EntityContainer>()->AddChild(entity);

   return entity;
}

EntityPtr Room::CreateWall(EntityPtr post1, EntityPtr post2, const csg::Point3& normal)
{
   om::EntityPtr entity = GetStore().AllocObject<Entity>();
 
   entity->SetDebugName("wall for room " + stdutil::ToString(GetEntity().GetEntityId()));
   auto mob = entity->AddComponent<Mob>();
   mob->SetInterpolateMovement(false);

   entity->AddComponent<Wall>()->Create(post1->GetComponent<Post>(),
                                        post2->GetComponent<Post>(),
                                        normal);
   
   GetEntity().AddComponent<EntityContainer>()->AddChild(entity);

   return entity;
}

void Room::CreateFloor()
{
   om::EntityPtr entity = Stonehearth::CreateEntity(GetStore(), "module://stonehearth/buildings/floor");

   entity->SetDebugName("floor for room " + stdutil::ToString(GetEntity().GetEntityId()));
   auto mob = entity->AddComponent<Mob>();
   mob->SetInterpolateMovement(false);

   // Floors live at (1, 0, 1), which is diagonally adjacent to the post which lives
   // at the origin.
   mob->MoveToGridAligned(math3d::ipoint3(1, 0, 1));

   // Create the floor component and add the floor entity to our container
   entity->AddComponent<Floor>()->Create();
   GetEntity().AddComponent<EntityContainer>()->AddChild(entity);

   floor_ = entity;
}

void Room::CreateRoof()
{
   om::EntityPtr entity = Stonehearth::CreateEntity(GetStore(), "module://stonehearth/buildings/roof");

   entity->SetDebugName("roof for room " + stdutil::ToString(GetEntity().GetEntityId()));
   auto mob = entity->AddComponent<Mob>();
   mob->SetInterpolateMovement(false);

   // Peaked roofs are positioned slightly lower than the height of the room
   // so they fit over the walls like a top-hat.
   mob->MoveToGridAligned(math3d::ipoint3(-1, RoomHeight - 1, -1));

   // Create the roof component and add the roof entity to our container
   entity->AddComponent<PeakedRoof>()->Create();
   GetEntity().AddComponent<EntityContainer>()->AddChild(entity);

   roof_ = entity;
}

void Room::Create()
{
   // Create the 4 new posts
   for (int i = 0; i < 4; i++) {
      posts_[i] = CreatePost();
   }
   walls_[TOP] = CreateWall(posts_[TOP_LEFT], posts_[TOP_RIGHT], csg::Point3(0, 0, -1));
   walls_[LEFT] = CreateWall(posts_[TOP_LEFT], posts_[BOTTOM_LEFT], csg::Point3(-1, 0, 0));
   walls_[RIGHT] = CreateWall(posts_[TOP_RIGHT], posts_[BOTTOM_RIGHT], csg::Point3(1, 0, 0));
   walls_[BOTTOM] = CreateWall(posts_[BOTTOM_LEFT], posts_[BOTTOM_RIGHT], csg::Point3(0, 0, 1));

   CreateFloor();
   CreateRoof();
}

void Room::ResizeRoom(const csg::Point3& size)
{
   dimensions_.Modify() = size;

   // The floor is 2 units smaller than the room, since posts and walls
   // live on the perimeter.
   csg::Point3 floorSize(0, 0, 0), roofSize(size.x + 2, 1, size.z + 2);
   if (size.x > 2 && size.z > 2) {
      floorSize.x = size.x - 2;
      floorSize.y = 1;
      floorSize.z = size.z - 2;
   }
   auto roof = (*roof_)->GetComponent<PeakedRoof>();
   (*floor_)->GetComponent<Floor>()->Resize(floorSize);
   roof->UpdateShape(roofSize);

   csg::Region3 roofRegion = roof->GetRegion();
   roofRegion.Translate(GetEntityWorldGridLocation(roof));

   auto move = [=](EntityPtr e, const csg::Point3&pt ) {
      e->GetComponent<Mob>()->MoveToGridAligned(pt);
   };

   move(posts_[TOP_LEFT], csg::Point3(0, 0, 0));
   move(posts_[TOP_RIGHT], csg::Point3(size[0] - 1, 0, 0));
   move(posts_[BOTTOM_LEFT], csg::Point3(0, 0, size[2] - 1));
   move(posts_[BOTTOM_RIGHT], csg::Point3(size[0] - 1, 0, size[2] - 1));

   for (int i = 0; i < 4; i++) {
      walls_[i]->GetComponent<Wall>()->Resize(roofRegion);
   }
}
