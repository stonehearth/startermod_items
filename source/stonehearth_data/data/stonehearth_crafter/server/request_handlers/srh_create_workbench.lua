local Point3 = _radiant.csg.Point3
local CreateWorkbench = class()

-- server side object to handle creation of the workbench.  this is called
-- by doing a POST to the route for this file specified in the manifest.

function CreateWorkbench:handle_request(session, postdata)
   -- pull the location and entity uri out of the postdata, create that
   -- entity, and move it there.
   local location = Point3(postdata.location.x, postdata.location.y, postdata.location.z)
   local workbench_entity = radiant.entities.create_entity(postdata.workbench_entity)
   local workshop_component = workbench_entity:get_component('stonehearth_crafter:workshop')

   -- Plasde the bench in the world
   radiant.terrain.place_entity(workbench_entity, location)

   -- Place the promotion talisman on the workbench, if there is one
   local promotion_talisman_entity, outbox_entity = workshop_component:init_from_scratch()

   -- set the faction of the bench and talisman
   workbench_entity:get_component('unit_info'):set_faction(session.faction)
   promotion_talisman_entity:get_component('unit_info'):set_faction(session.faction)
   outbox_entity:get_component('unit_info'):set_faction(session.faction)

   -- return the entity, in case the client cares (e.g. if they want to make that
   -- the selected item)
   return { workbench_entity = workbench_entity }
end


return CreateWorkbench