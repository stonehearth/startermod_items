local Entity = _radiant.om.Entity
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_rng()

local INFINITE = 1000000

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
   if entity and entity:is_valid() then
      radiant.entities.remove_child(radiant._root_entity, entity)
      radiant.entities.move_to(entity, Point3.zero)
   end
end

function Terrain.get_standable_point(entity, pt)
   local start_point = Terrain.get_point_on_terrain(pt)
   return _physics:get_standable_point(entity, start_point)
end

-- returns the Point3 of the highest terrain voxel at (x, z)
function Terrain.get_point_on_terrain(pt)
   return Terrain._get_terrain_component():get_point_on_terrain(pt)
end

function Terrain.in_bounds(pt)   
   -- the terrain bounds contains the bounds of all the tiles which have been
   -- defined.  we'll add in some extra padding on the top representing the
   -- sky.  otherwise, the point directly above the highest mountain in the world
   -- would be classified as "out of bounds"
   local bounds = Terrain._get_terrain_component():get_bounds()
   bounds.max.y = INFINITE
   return bounds:contains(pt:to_closest_int())
end

-- returns whether an entity can stand on the Point3 location
-- @param arg0 - the entity in question
-- @param arg1 - the location
function Terrain.is_standable(arg0, arg1)
   if arg1 == nil then
      local location = arg0
      assert(radiant.util.is_a(location, Point3))
      return _physics:is_standable(location)
   end
   local entity, location = arg0, arg1
   assert(radiant.util.is_a(entity, Entity))
   assert(radiant.util.is_a(location, Point3))
   return _physics:is_standable(entity, location)
end

-- returns whether an entity can stand on the Point3 location
function Terrain.is_supported(location)
   assert(radiant.util.is_a(location, Point3))
   return _physics:is_supported(location)
end

-- returns whether an entity can stand occupy location
function Terrain.is_blocked(arg0, arg1)
   if arg1 == nil then
      local location = arg0
      assert(radiant.util.is_a(location, Point3))
      return _physics:is_blocked(location)
   end
   local entity, location = arg0, arg1
   assert(radiant.util.is_a(entity, Entity))
   assert(radiant.util.is_a(location, Point3))
   return _physics:is_blocked(entity, location)
end

function Terrain.is_terrain(location)
   assert(radiant.util.is_a(location, Point3))
   return _physics:is_terrain(location)
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
   local entities = _physics:get_entities_in_region(region)
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

-- only finds points at the same elevation as origin
function Terrain.find_placement_point(origin, min_radius, max_radius)
   -- pick a random start location
   local x = rng:get_int(-max_radius, max_radius)
   local z = rng:get_int(-max_radius, max_radius)

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
      -- if z inside min_radius
      if z > -min_radius and z < min_radius then
         -- return whether x is on or outside min_radius
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
   local diameter = 2*max_radius + 1 -- add 1 to include origin square
   local fallback_point = nil

   for i=1, diameter*diameter do
      if valid(x, z) then
         pt = origin + Point3(x, 0, z)

         if _physics:is_standable(pt) then
            if not _physics:is_occupied(pt) then
               found = true
               break
            else
               if not fallback_point then
                  fallback_point = pt
               end
            end
         end
      end
      x, z = inc(x, z)
   end

   if not found then
      -- fallback to a standable point, but still indicate not found
      pt = fallback_point or origin
   end

   return pt, found
end

function Terrain.is_valid_direct_path(from, to, entity, reversible)
   local end_point = Terrain.get_direct_path_end_point(from, to, entity, reversible)
   return end_point == to
end

function Terrain.get_direct_path_end_point(from, to, entity, reversible)
   if reversible == nil then
      reversible = true
   end

   local end_point

   local direct_path_finder = _radiant.sim.create_direct_path_finder(entity)
      :set_start_location(from)
      :set_end_location(to)
      :set_allow_incomplete_path(true)
      :set_reversible_path(reversible)

   local path = direct_path_finder:get_path()
   if path and not path:is_empty() then
      end_point = path:get_finish_point()
   else
      end_point = from
   end

   return end_point
end

-- only finds points at the same elevation as location
function Terrain.find_closest_standable_point_to(location, max_radius, entity)
   local fallback_point = nil

   local check_point = function(point)
         if _physics:is_standable(entity, point) then
            if not _physics:is_occupied(entity, point) then
               return true
            else
               if not fallback_point then
                  fallback_point = location
               end
            end
         end
      end

   if check_point(location) then
      return location, true
   end

   -- randomize signs so we don't favor the top-left corner
   local sx = rng:get_int(0, 1)*2 - 1
   local sz = rng:get_int(0, 1)*2 - 1
   local point = Point3(0, location.y, 0)

   for radius = 1, max_radius do
      -- apply the randomized iteration direction
      local rx = radius * sx
      local rz = radius * sz

      -- top and bottom edges
      for z = -rz, rz, 2*rz do
         for x = -rx, rx, sx do
            point.x = location.x + x
            point.z = location.z + z
            if check_point(point) then
               return point, true
            end
         end
      end

      -- left and right edges
      for z = -rz+sz, rz-sz, sz do  -- +/- sz so we skip points on the top and bottom eedges that were already checked
         for x = -rx, rx, 2*rx do
            point.x = location.x + x
            point.z = location.z + z
            if check_point(point) then
               return point, true
            end
         end
      end
   end

   fallback_point = fallback_point or location

   -- return the fallback_point and a flag indicating failure
   return fallback_point, false
end

function Terrain.add_point(point, tag)
   Terrain.add_cube(Cube3(point, Point3(point.x+1, point.y+1, point.z+1), tag))
end

function Terrain.add_cube(cube)
   Terrain._get_terrain_component():add_cube(cube)
end

function Terrain.add_region(region)
   Terrain._get_terrain_component():add_region(region)
end

function Terrain.subtract_point(point)
   Terrain.subtract_cube(Cube3(point, Point3(point.x+1, point.y+1, point.z+1)))
end

function Terrain.subtract_cube(cube)
   Terrain._get_terrain_component():subtract_cube(cube)
end

function Terrain.subtract_region(region)
   Terrain._get_terrain_component():subtract_region(region)
end

function Terrain.intersect_cube(cube)
   return Terrain._get_terrain_component():intersect_cube(cube)
end

function Terrain.intersect_region(region)
   return Terrain._get_terrain_component():intersect_region(region)
end

function Terrain._get_terrain_component()
   return radiant._root_entity:add_component('terrain')
end

function Terrain.get_movement_cost_at(point)
   return _physics:get_movement_cost_at(point)
end

return Terrain
