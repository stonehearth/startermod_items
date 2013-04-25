#include "pch.h"
#include "build_orders.h"
#include "scaffolding.h"
#include "room.h"

using namespace ::radiant;
using namespace ::radiant::om;

void BuildOrders::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("projects",   projects_);
   AddRecordField("blueprints",  blueprints_);
   AddRecordField("inprogress",  inprogress_);
   AddRecordField("running_projects",  runningProjects_);
}

void BuildOrders::AddBlueprint(EntityRef b)
{
   blueprints_.Insert(b);
}

bool BuildOrders::StartProject(EntityRef b, EntityRef p)
{
   EntityPtr blueprint = b.lock();
   EntityPtr project = p.lock();

   if (blueprint && project) {
      if (!runningProjects_.Contains(blueprint)) {
         dm::CloneMapping mapping;

         ASSERT(project->GetComponents().IsEmpty());

         //LOG(WARNING) << "blueprint: " << std::endl << dm::Format<EntityPtr>(blueprint);
         blueprint->CloneDynamicObject(project, mapping);

         //LOG(WARNING) << "project: " << std::endl << dm::Format<EntityPtr>(project);

         // xxx - not sure we need these... hold onto them anyway
         runningProjects_.Insert(blueprint);
         projects_.Insert(project);

         // Add all the sub-projects
         for (const auto& entry : mapping.dynamicObjects) {
            switch (entry.second->GetObjectType()) {
               case FixtureObjectType:
               case WallObjectType:
               case FloorObjectType:
               case PeakedRoofObjectType:
               case PostObjectType: {
                  BuildOrderPtr ps = std::dynamic_pointer_cast<BuildOrder>(entry.second);
                  inprogress_.Insert(ps->GetEntityRef());
                  break;
               }
            }
         }

         // Start the project
         auto room = project->GetComponent<Room>();
         ASSERT(room);
         room->StartProject(mapping, [=](BuildOrderPtr r) { inprogress_.Insert(r->GetEntityRef()); });
         return true;
      }
   }
   return false;
}
