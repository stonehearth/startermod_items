local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3

local Terrain = {}
local singleton = {}

-- place an entity in the world.  this may not end up no the terrain, but is
-- the most appropriate place given the specified `location`.  for example,
-- if there's floor on the place that you specified, this will put the item
-- on the floor (not inside it!)
--
--   @param entity - the entity to place
--   @param location - where to place it.  the entity will be placed at the
--                     most appropriate y coordinate based on location.
--
function Terrain.place_entity(entity, location)
   if type(location) == "table" then
      location = Point3(location.x, location.y, location.z)
   end
   location = Terrain.get_standable_point(entity, location)
   return Terrain.place_entity_at_exact_location(entity, location)
end

-- place `entity` at the exact spot specified by `location`.  this could
-- be floating in the air or underground... it's up to you! 
--
--   @param entity - the entity to place
--   @param location - the exact position in the world to put it
--
function Terrain.place_entity_at_exact_location(entity, location)
   local render_info = entity:add_component('render_info')
   local variant = render_info:get_model_variant()
   if variant == '' then
      render_info:set_model_variant('iconic')
   end
   radiant.entities.add_child(radiant._root_entity, entity, location)
end

function Terrain.remove_entity(entity)
   radiant.entities.remove_child(radiant._root_entity, entity)
end

function Terrain.get_standable_point(entity, pt)
   local start_point = Terrain.get_point_on_terrain(pt)
   return _physics:get_standable_point(entity, start_point)
end

-- returns the height (y coordinate) of the highest terrain voxel at (x, z)
function Terrain.get_point_on_terrain(pt)
   return radiant._root_entity:add_component('terrain')
                                 :get_point_on_terrain(pt)
end

-- returns whether an entity can stand on the Point3 location
function Terrain.is_standable(entity, location)
   return _physics:is_standable(entity, location)
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

function Terrain.get_entities_in_region(region, filter_fn)
   local entities = {}
   for cube in region:each_cube() do
      for id, entity in pairs(Terrain.get_entities_in_cube(cube,filter_fn)) do
         entities[id] = entity
      end
   end
   return entities
end

function Terrain.get_entities_at_point(point, filter_fn)
   local cube = Cube3(point, point + Point3(1, 1, 1))
   return Terrain.get_entities_in_cube(cube, filter_fn)
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
