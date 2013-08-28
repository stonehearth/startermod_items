local Point3 = _radiant.csg.Point3

local Terrain = {}

local _terrain = radiant._root_entity:add_component('terrain')

function Terrain.add_cube(cube)
   _terrain:add_cube(cube)
end

function Terrain.place_entity(entity, location)
   radiant.entities.add_child(radiant._root_entity, entity, location)
   local render_info = entity:add_component('render_info')
   local variant = render_info:get_model_variant()
   if variant == '' then
      render_info:set_model_variant('iconic')
   end

   if type(location) == "table" then
      location = Point3(location.x, location.y, location.z)
   end  
   _terrain:place_entity(entity, location)
end

function Terrain.trace_world_entities(reason, added_cb, removed_cb)
   local ec = radiant.entities.get_root_entity():add_component('entity_container');
   local children = ec:get_children()

   -- put a trace on the root entity container to detect when items 
   -- go on and off the terrain.  each item is forwarded to the
   -- appropriate tracker.
   local trace = children:trace('radiant.terrain: ' .. reason)
                     :on_added(added_cb)
                     :on_removed(removed_cb)
   for id, entity in children:items() do
      added_cb(id, entity)
   end  
   return trace
end

function Terrain.get_world_entities()
   local ec = radiant.entities.get_root_entity():add_component('entity_container');
   local children = ec:get_children()
   return children:items()
end

return Terrain
