local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3

local Terrain = {}
local singleton = {}

function Terrain.place_entity(entity, location)
   local render_info = entity:add_component('render_info')
   local variant = render_info:get_model_variant()
   if variant == '' then
      render_info:set_model_variant('iconic')
   end

   radiant.entities.add_child(radiant._root_entity, entity, location)
   if location then
      if type(location) == "table" then
         location = Point3(location.x, location.y, location.z)
      end
      local terrain = radiant._root_entity:add_component('terrain')
      terrain:place_entity(entity, location)
   end
end

function Terrain.remove_entity(entity)
   radiant.entities.remove_child(radiant._root_entity, entity)
end

-- returns the height (y coordinate) of the highest terrain voxel at (x, z)
function Terrain.get_height(x, z)
   local terrain = radiant._root_entity:add_component('terrain')

   if not terrain:in_bounds(Point3(x, 0, z)) then
      return nil
   end

   return terrain:get_height(x, z)
end

-- returns whether an entity can stand on the Point3 location
function Terrain.can_stand_on(entity, location)
   return _physics:can_stand_on(entity, location)
end

-- returns all entities whose locations of collision shapes intersect the cube
function Terrain.get_entities_in_cube(cube, filter_fn)
   local entities = _physics:get_entities_in_cube(cube)
   if filter_fn then
      for id, entity in pairs(entities) do         
         if not filter_fn(entity) then
            entities[id] = nil
         end
      end
   end
   return entities
end

function Terrain.get_entities_at_point(point, filter_fn)
   return Terrain.get_entities_in_cube(Cube3(point, point + Point3(1, 1, 1)))
end

function Terrain.trace_world_entities(reason, added_cb, removed_cb)
   local root = radiant.entities.get_root_entity()
   local ec = radiant.entities.get_root_entity():add_component('entity_container');

   -- put a trace on the root entity container to detect when items
   -- go on and off the terrain.  each item is forwarded to the
   -- appropriate tracker.
   return ec:trace_children('radiant.terrain: ' .. reason)
                        :on_added(function (id, entity) 
                              if entity and entity:is_valid() then
                                 added_cb(id, entity)
                              end
                           end)
                        :on_removed(removed_cb)
                        :push_object_state()
end

return Terrain
