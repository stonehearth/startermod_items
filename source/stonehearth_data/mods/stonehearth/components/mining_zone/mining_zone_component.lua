local csg_lib = require 'lib.csg.csg_lib'
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local TraceCategories = _radiant.dm.TraceCategories
local log = radiant.log.create_logger('mining')

local MiningZoneComponent = class()

local MAX_REACH_DOWN = 1
local MAX_REACH_UP = 3
local MAX_DESTINATION_DELTA_Y = 3

local DIRECTIONS = {
    Point3.unit_y,
   -Point3.unit_y,
    Point3.unit_x,
   -Point3.unit_x,
    Point3.unit_z,
   -Point3.unit_z
}

local NON_TOP_DIRECTIONS = {
    Point3.unit_x,
   -Point3.unit_x,
    Point3.unit_z,
   -Point3.unit_z,
   -Point3.unit_y
}

local SIDE_DIRECTIONS = {
    Point3.unit_x,
   -Point3.unit_x,
    Point3.unit_z,
   -Point3.unit_z
}

function MiningZoneComponent:__init()
end

function MiningZoneComponent:initialize(entity, json)
   self._entity = entity
   self._destination_component = self._entity:add_component('destination')
   self._collision_shape_component = self._entity:add_component('region_collision_shape')
   self._last_destination_region = Region3()
   self._last_unreserved_region = Region3()

   self._sv = self.__saved_variables:get_data()
   if not self._sv.initialized then
      self._collision_shape_component
         :set_region_collision_type(_radiant.om.RegionCollisionShape.NONE)

      self._destination_component
         :set_region(_radiant.sim.alloc_region3())
         :set_adjacent(_radiant.sim.alloc_region3())
         :set_reserved(_radiant.sim.alloc_region3())

      self._sv.designation_entity = self:_create_designation_entity()

      -- do this last as it fires off the region changed events
      self:set_region(_radiant.sim.alloc_region3())

      self._sv.enabled = true
      self._sv.selectable = true
      self._sv.initialized = true
      self.__saved_variables:mark_changed()
   else
      self:_restore()
   end

   self:_trace_location()
   self:_trace_reserved()

   -- TODO: listen for changes to terrain
end

function MiningZoneComponent:_restore()
   self._destination_component:get_reserved():modify(function(cursor)
         cursor:clear()
      end)
   
   self:_trace_region()
   self:_update_mining_task()
end

function MiningZoneComponent:destroy()
   self:_destroy_mining_task()
   self:_destroy_region_trace()
   self:_destroy_reserved_trace()
   self:_destroy_location_trace()

   radiant.entities.destroy_entity(self._sv.designation_entity)
end

-- region is a boxed region
function MiningZoneComponent:get_region()
   return self._sv.region
end

-- region is a boxed region
function MiningZoneComponent:set_region(region)
   region:modify(function(cursor)
         cursor:optimize_by_merge('new mining zone region')
      end)

   self._sv.region = region
   self:_trace_region()
   self.__saved_variables:mark_changed()

   -- have the collision shape use the same region
   self._collision_shape_component:set_region(self._sv.region)

   return self
end

function MiningZoneComponent:mine_point(point)
   local block_kind = radiant.terrain.get_block_kind_at(point)
   local loot = stonehearth.mining:roll_loot(block_kind)

   stonehearth.mining:mine_point(point)
   self:_update_destination()

   if self._destination_component:get_region():get():empty() then
      radiant.entities.destroy_entity(self._entity)
   end

   return loot
end

function MiningZoneComponent:get_enabled()
   return self._sv.enabled
end

function MiningZoneComponent:set_enabled(enabled)
   if self._sv.enabled == enabled then
      return
   end

   self._sv.enabled = enabled
   self.__saved_variables:mark_changed()

   self:_update_mining_task()
end

function MiningZoneComponent:set_enabled_command(session, response, enabled)
   self:set_enabled(enabled)
   return {}
end

function MiningZoneComponent:get_selectable()
   return self._sv.selectable
end

function MiningZoneComponent:set_selectable(selectable)
   self._sv.selectable = selectable
   self.__saved_variables:mark_changed()
end

function MiningZoneComponent:_trace_region()
   self:_destroy_region_trace()

   self._region_trace = self._sv.region:trace('mining zone')
      :on_changed(function()
            self:_on_region_changed()
         end)
      :push_object_state()
end

function MiningZoneComponent:_destroy_region_trace()
   if self._region_trace then
      self._region_trace:destroy()
      self._region_trace = nil
   end
end

function MiningZoneComponent:_on_region_changed()
   -- cache the region bounds.  force optimize before caching to make
   -- sure we absolutely have the minimal region.  not having the smallest
   -- region possible will have cascading performance problems down the
   -- line.
   self._sv.region:modify(function(cursor)      
         cursor:force_optimize_by_merge('mining zone region changed')
      end)
   self.__saved_variables:mark_changed()

   self:_update_destination()
   self:_update_designation()
   self:_create_mining_task()
end

function MiningZoneComponent:_trace_location()
   self:_destroy_location_trace()

   self._location_trace = radiant.entities.trace_location(self._entity, 'mining zone location')
      :on_changed(function()
            self:_on_location_changed()
         end)
end

function MiningZoneComponent:_destroy_location_trace()
   if self._location_trace then
      self._location_trace:destroy()
      self._location_trace = nil
   end
end

function MiningZoneComponent:_on_location_changed()
   local location = radiant.entities.get_world_location(self._entity)
   radiant.terrain.place_entity_at_exact_location(self._sv.designation_entity, location)
end

function MiningZoneComponent:_trace_reserved()
   self:_destroy_reserved_trace()

   self._reserved_trace = self._destination_component:trace_reserved('mining zone component', TraceCategories.SYNC_TRACE)
      :on_changed(function()
            self:_on_reserved_changed()
         end)
end

function MiningZoneComponent:_destroy_reserved_trace()
   if self._reserved_trace then
      self._reserved_trace:destroy()
      self._reserved_trace = nil
   end
end

function MiningZoneComponent:_on_reserved_changed()
   self:_update_adjacent()
end

local function face_is_exposed(point, direction)
   return not radiant.terrain.is_terrain(point + direction)
end

local function count_open_faces_for_block(point, directions, max)
   max = max or 6
   local count = 0

   for _, dir in ipairs(directions) do
      if not radiant.terrain.is_terrain(point + dir) then
         count = count + 1
         if count >= max then
            return count
         end
      end
   end

   return count
end

local function has_n_plus_exposed_faces(point, directions, n)
   local count = count_open_faces_for_block(point, directions, n)
   return count >= n
end

local function has_higher_neighbor(point, radius)
   local test_point = Point3()
   local px = point.x
   local pz = point.z
   local y = point.y + 1
   
   for z = -radius, radius do
      for x = -radius, radius do
         if x ~= 0 or y ~= 0 then
            test_point:set(px + x, y, pz + z)
            if radiant.terrain.is_terrain(test_point) then
               return true
            end
         end
      end
   end
   return false
end

-- To make sure we mine in layers instead of digging randomly and potentially orphaning regions,
-- we prioritize blocks with two or more exposed faces.
function MiningZoneComponent:_update_destination()
   local location = radiant.entities.get_world_grid_location(self._entity)
   if not location then
      return
   end

   local zone_region = self._sv.region:get()

   self._destination_component:get_region():modify(function(cursor)
         cursor:clear()

         -- break the zone into convex regions (cubes) and run the destination block algorithm
         -- assumes the zone_region has been optimized already
         for zone_cube in zone_region:each_cube() do
            self:_add_destination_blocks(cursor, zone_cube, location)
         end

         -- mostly for debugging
         cursor:optimize_by_merge('mining:_update_destination()')
      end)

   self:_update_adjacent()
end

-- get the unreserved terrain region that lies inside the zone_cube
function MiningZoneComponent:_get_working_region(zone_cube, zone_location)
   local reserved_region = self._destination_component:get_reserved():get()
   local zone_region = Region3(zone_cube)
   local zone_reserved_region = zone_region:intersect_region(reserved_region)

   zone_region:subtract_region(zone_reserved_region)
   zone_region:translate(zone_location)
   zone_reserved_region:translate(zone_location)

   local working_region = radiant.terrain.intersect_region(zone_region)
   working_region:set_tag(0)
   working_region:optimize_by_merge('mining:_get_working_region()')
   return working_region, zone_reserved_region
end

-- this algorithm assumes a convex region, so we break the zone into cubes before running it
-- working_region and zone_reserved_region are in world coordinates
-- destination_region and zone_cube are in local coordinates
function MiningZoneComponent:_add_destination_blocks(destination_region, zone_cube, zone_location)
   local up = Point3.unit_y
   local down = -Point3.unit_y
   local one = Point3.one
   local world_to_local_translation = -zone_location
   local working_region, zone_reserved_region = self:_get_working_region(zone_cube, zone_location)
   local working_bounds = working_region:get_bounds()
   local unsupported_blocks = {}

   local add_block = function(point)
         local block = Cube3(point, point + one)
         block:translate(world_to_local_translation)
         destination_region:add_cube(block)
      end

   local try_add_block = function(point)
         if face_is_exposed(point, down) and face_is_exposed(point, up) then
            local num_exposed_sides = count_open_faces_for_block(point, SIDE_DIRECTIONS, 3)
            local block_info = { num_exposed_sides = num_exposed_sides, point = point }
            table.insert(unsupported_blocks, block_info)
         else
            add_block(point)
         end
      end

   self:_add_top_facing_blocks(working_region, working_bounds, try_add_block)
   self:_add_side_and_bottom_blocks(working_region, working_bounds, try_add_block)

   self:_add_unsupported_blocks(unsupported_blocks, add_block)

   if destination_region:empty() then
      -- fallback condition
      log:detail('adding all exposed faces to destination')
      self:_add_all_exposed_faces(working_region, add_block)
   end

   if destination_region:empty() then
      -- fallback condition
      log:detail('adding all blocks to destination')
      working_region:translate(world_to_local_translation)
      destination_region:add_region(working_region)
      working_region = nil -- don't reuse, not in world coordinates anymore
   end

   -- add the reserved region back, since we excluded it from the working set analysis
   for point in zone_reserved_region:each_point() do
      -- point may have been mined, but not yet unreserved
      if radiant.terrain.is_terrain(point) then
         add_block(point)
      end
   end
end

function MiningZoneComponent:_add_top_facing_blocks(working_region, working_bounds, add_block_fn)
   local up = Point3.unit_y
   
   local upper_bound_points = Region3()
   local other_points = Region3()

   for cube in working_region:each_cube() do
      if cube.max.y >= working_bounds.max.y - MAX_DESTINATION_DELTA_Y then
         local top_face = csg_lib.get_face(cube, up)

         if top_face.max.y == working_bounds.max.y then
            for point in cube:each_point() do
               upper_bound_points:add_point(point + up)
            end
         else
            for point in cube:each_point() do
               other_points:add_point(point + up)
            end
         end
      end
   end

   local res = _physics:clip_region(upper_bound_points, _radiant.physics.Physics.CLIP_TERRAIN)
   for point in res:each_point() do
      add_block_fn(point - up)
   end

   res = _physics:clip_region(other_points, _radiant.physics.Physics.CLIP_TERRAIN)
   for point in res:each_point() do
      local p = point - up
      if not has_higher_neighbor(p, 1) then
         add_block_fn(p)
      end
   end
end

function MiningZoneComponent:_add_side_and_bottom_blocks(working_region, working_bounds, add_block_fn)
   for _, normal in ipairs(NON_TOP_DIRECTIONS) do
      local face_region = Region3(csg_lib.get_face(working_bounds, normal))
      local face_blocks = face_region:intersect_region(working_region)
      face_blocks:translate(normal)
      local res = _physics:clip_region(face_blocks, _radiant.physics.Physics.CLIP_TERRAIN)
      res:translate(-normal)
      for point in res:each_point() do
         add_block_fn(point)
      end
      face_blocks:empty()
   end
end

function MiningZoneComponent:_add_unsupported_blocks(unsupported_blocks, add_block_fn)
   if not next(unsupported_blocks) then
      return
   end

   local compare_block = function(a, b)
      return a.num_exposed_sides > b.num_exposed_sides
   end

   table.sort(unsupported_blocks, compare_block)
   local cutoff = math.min(unsupported_blocks[1].num_exposed_sides, 3)

   for _, block_info in ipairs(unsupported_blocks) do
      if block_info.num_exposed_sides >= cutoff then
         add_block_fn(block_info.point)
      end
   end
end

function MiningZoneComponent:_add_all_exposed_faces(working_region, add_block_fn)
   for cube in working_region:each_cube() do
      csg_lib.each_face_block_in_cube(cube, function(point)
            if has_n_plus_exposed_faces(point, DIRECTIONS, 1) then
               add_block_fn(point)
            end
         end)
   end
end

-- Note that nothing in ths component is directly listening to terrain changes yet, so we will miss minable
-- points that open up because of external terrain modifications
function MiningZoneComponent:_update_adjacent()
   self:_update_adjacent_incremental()
end

-- slow version with an inner loop that is O(#_destination_blocks) / O(surface_area) of the mining region
function MiningZoneComponent:_update_adjacent_full()
   local location = radiant.entities.get_world_grid_location(self._entity)
   if not location then
      return
   end

   local unreserved_region = self:_get_unreserved_region()
   unreserved_region:translate(location)
   local adjacent = self:_calculate_adjacent(unreserved_region)
   adjacent:translate(-location)

   self._destination_component:get_adjacent():modify(function(cursor)
         cursor:clear()
         cursor:add_region(adjacent)
         cursor:optimize_by_merge('mining:_update_adjacent_full()')
      end)
end

-- fast version that is O(1) on the size of the mining region
function MiningZoneComponent:_update_adjacent_incremental()
   local location = radiant.entities.get_world_grid_location(self._entity)
   if not location then
      return
   end

   local max_reach_y = math.max(MAX_REACH_UP, MAX_REACH_DOWN)
   local reach = Point3(1, max_reach_y, 1)
   local destination_region = self._destination_component:get_region():get()
   local unreserved_region = self:_get_unreserved_region()
   local destination_delta = self:_get_region_delta(self._last_destination_region, destination_region)
   local unreserved_delta = self:_get_region_delta(self._last_unreserved_region, unreserved_region)

   -- the set of changed blocks that will affect the adjacent
   local changed_blocks = destination_delta + unreserved_delta

   -- the adjacent blocks which are affected by the changed blocks
   -- TODO: perform a separable inflation to exclude diagonals
   local dirty_adjacent = changed_blocks:inflated(reach)

   -- destination volume that can affect the dirty adjacent region
   -- TODO: perform a separable inflation to exclude diagonals
   local dirty_unreserved_volume = dirty_adjacent:inflated(reach)

   -- destination blocks that need to be recalculated
   local dirty_unreserved = dirty_unreserved_volume:intersect_region(unreserved_region)

   -- calculate the updated adjacent volume 
   dirty_unreserved:translate(location)
   local adjacent_fragment = self:_calculate_adjacent(dirty_unreserved)
   adjacent_fragment:translate(-location)

   self._destination_component:get_adjacent():modify(function(cursor)
         cursor:subtract_region(dirty_adjacent)
         cursor:add_region(adjacent_fragment)
         cursor:optimize_by_merge('mining:_update_adjacent_incremental()')
      end)

   self._last_unreserved_region = unreserved_region
   self._last_destination_region = Region3(destination_region)

   -- for verifying the calculation between the fast and slow versions
   -- these can also differ if an external party modified the terrain
   local verify_adjacent = false
   if verify_adjacent then
      local incremental_adjacent = Region3(self._destination_component:get_adjacent():get())
      self:_update_adjacent_full()
      local full_adjacent = self._destination_component:get_adjacent():get()
      local intersection = full_adjacent:intersect_region(incremental_adjacent)
      assert(intersection:get_area() == full_adjacent:get_area())
   end
end

function MiningZoneComponent:_get_unreserved_region()
   local destination_region = self._destination_component:get_region():get()
   local reserved_region = self._destination_component:get_reserved():get()
   local unreserved_region = destination_region - reserved_region
   unreserved_region:optimize_by_merge('mining:_get_unreserved_region()') -- probably unnecessary
   return unreserved_region
end

function MiningZoneComponent:_calculate_adjacent(world_region)
   local adjacent = Region3()

   for point in world_region:each_point() do
      local adjacent_region = stonehearth.mining:get_adjacent_for_destination_block(point)
      adjacent:add_region(adjacent_region)
   end

   return adjacent
end

function MiningZoneComponent:_get_region_delta(region1, region2)
   local added_to_region1 = region2 - region1
   local removed_from_region1 = region1 - region2
   local delta = added_to_region1 + removed_from_region1
   return delta
end

function MiningZoneComponent:_create_mining_task()
   local town = stonehearth.town:get_town(self._entity)
   
   self:_destroy_mining_task()

   if self._sv.region:get():empty() then
      return
   end

   self._mining_task = town:create_task_for_group('stonehearth:task_group:mining',
                                                  'stonehearth:mining:mine',
                                                  {
                                                     mining_zone = self._entity
                                                  })
      :set_source(self._entity)
      :set_name('mine task')
      :set_priority(stonehearth.constants.priorities.mining.MINE)
      :set_max_workers(4)
      :notify_completed(function()
            self._mining_task = nil
         end)
      :start()
end

function MiningZoneComponent:_destroy_mining_task()
   if self._mining_task then
      self._mining_task:destroy()
      self._mining_task = nil
   end
end

function MiningZoneComponent:_update_mining_task()
   if self._sv.enabled then
      self:_create_mining_task()
   else
      self:_destroy_mining_task()
   end
end

function MiningZoneComponent:_create_designation_entity()
   local entity = radiant.entities.create_entity('stonehearth:mining_zone_designation', { owner = self._entity })
   local collision_shape_component = entity:add_component('region_collision_shape')
      :set_region_collision_type(_radiant.om.RegionCollisionShape.NONE)
      :set_region(_radiant.sim.alloc_region3())
   return entity
end

function MiningZoneComponent:_update_designation()
   local collision_shape_component = self._sv.designation_entity:add_component('region_collision_shape')
   collision_shape_component:get_region():modify(function(cursor)
         cursor:clear()

         -- make designation region extend 1 block above the zone
         for cube in self._sv.region:get():each_cube() do
            local extended_cube = Cube3(cube)
            extended_cube.max.y = extended_cube.max.y + 1
            cursor:add_cube(extended_cube)
         end

         cursor:optimize_by_merge('mining:_update_designation()')
      end)
end

return MiningZoneComponent
