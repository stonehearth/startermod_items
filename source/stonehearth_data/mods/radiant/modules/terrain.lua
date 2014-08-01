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
function Terrain.place_entity(entity, location, options)
   if type(location) == "table" then
      location = Point3(location.x, location.y, location.z)
   end
   local pt = _physics:get_standable_point(entity, location)
   return Terrain.place_entity_at_exact_location(entity, pt, options)
end

-- place `entity` at the exact spot specified by `location`.  this could
-- be floating in the air or underground... it's up to you! 
--
--   @param entity - the entity to place
--   @param location - the exact position in the world to put it
--
function Terrain.place_entity_at_exact_location(entity, location, options)
   local force_iconic = not options or options.force_iconic ~= false

   if force_iconic then
      -- switch to the iconic form of an entity when placing on the ground.
      local entity_forms = entity:get_component('stonehearth:entity_forms')
      if entity_forms then
         local iconic_entity = entity_forms:get_iconic_entity()
         if iconic_entity then
            entity = iconic_entity
         end
      end
   end
   radiant.entities.add_child(radiant._root_entity, entity, location)
end

function Terrain.remove_entity(entity)
   radiant.entities.remove_child(radiant._root_entity, entity)
   radiant.entities.move_to(entity, Point3.zero)
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

-- returns whether an entity can stand occupy location
function Terrain.is_blocked(entity, location)
   return _physics:is_blocked(entity, location)
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
end

function Terrain.find_placement_point(starting_location, min_radius, max_radius)
   -- pick a random start location
   local x = math.random(-max_radius, max_radius)
   local z = math.random(-max_radius, max_radius)

   -- move to the next point in the box defined by max_radius
   local function inc(x, z)
      x = x + 1
      if x > max_radius then
         x, z = -max_radius, z + 1
      end
      if z > max_radius then
         z = -min_radius
      end
      return x, z
   end

   -- make sure x, z is inside the donut described by min_radius and max_radius   
   local function valid(x, z)
      if z >= -min_radius and z <= min_radius then
         return x <= -min_radius or x >= min_radius
      end
      return true
   end

   -- run through all the points in the box.  for the ones that are in the donut,
   -- see if they're both capable holding an item and not-occupied.  if we loop
   -- all the way around and still can't find something, just use the starting
   -- point as the placement point.
   local pt
   local found = false
   for i=1,max_radius * max_radius do
      x, z = inc(x, z)
      if valid(x, z) then
         pt = starting_location + Point3(x, 0, z)
         if _physics:is_standable(pt) and not _physics:is_occupied(pt) then
            found = true
            break
         end
      end
   end
   return pt, found
end

return Terrain
