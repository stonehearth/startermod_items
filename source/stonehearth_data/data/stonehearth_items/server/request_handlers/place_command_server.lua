local Point3 = _radiant.csg.Point3
local PlaceItemServer = class()

-- server side object to handle creation of the workbench.  this is called
-- by doing a POST to the route for this file specified in the manifest.

function PlaceItemServer:handle_request(session, postdata)

   -- pull the location and entity uri out of the postdata, create that
   -- entity, and move it there.
   local location = Point3(postdata.location.x, postdata.location.y, postdata.location.z)
   --local item_entity = radiant.entities.create_entity(postdata.item_uri)
   radiant.log.info('in server, placing item in the world!!! %s', postdata.proxy_id)

   local proxy_entity = radiant.entities.get_entity(tonumber(postdata.proxy_id))


   local item_entity = proxy_entity:get_component('stonehearth_items:placeable_item_proxy'):get_full_sized_entity()
   radiant.entities.turn_to(item_entity, postdata.curr_rotation)

   -- Remove the icon (TODO: what about stacked icons? Remove just one of the stack?)
   radiant.terrain.remove_entity(proxy_entity)

   -- Place the item in the world
   radiant.terrain.place_entity(item_entity, location)
   --item_entity:get_component('unit_info'):set_faction(session.faction)
   return { item_entity = item_entity }
end


return PlaceItemServer