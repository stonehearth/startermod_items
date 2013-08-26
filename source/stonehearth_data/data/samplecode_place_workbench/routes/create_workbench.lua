local RadiantIPoint3 = _radiant.csg.Point3
local CreateWorkbench = class()

-- server side object to handle creation of the workbench.  this is called
-- by doing a POST to the route for this file specified in the manifest.

function CreateWorkbench:handle_request(query, postdata)
   -- pull the location and entity uri out of the postdata, create that
   -- entity, and move it there.
   local location = RadiantIPoint3(postdata.location[1], postdata.location[2], postdata.location[3])
   local entity = radiant.entities.create_entity(postdata.entity)
   radiant.terrain.place_entity(entity, location)

   -- return the entity, in case the client cares (e.g. if they want to make that
   -- the selected item)
   return { entity = entity }
end


return CreateWorkbench