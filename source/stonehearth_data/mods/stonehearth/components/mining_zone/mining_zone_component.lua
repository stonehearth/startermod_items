local mining_lib = require 'lib.mining.mining_lib'
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local TraceCategories = _radiant.dm.TraceCategories
local log = radiant.log.create_logger('mining')

local MiningZoneComponent = class()

local MAX_REACH_DOWN = 1
local MAX_REACH_UP = 3
local MAX_DESTINATION_DELTA_Y = 3

local FACE_DIRECTIONS = {
    Point3.unit_y,
   -Point3.unit_y,
    Point3.unit_z,
   -Point3.unit_z,
    Point3.unit_x,
   -Point3.unit_x
}

local NON_TOP_DIRECTIONS = {
    Point3.unit_z,
   -Point3.unit_z,
    Point3.unit_x,
   -Point3.unit_x,
   -Point3.unit_y
}

function MiningZoneComponent:__init()
end

function MiningZoneComponent:initialize(entity, json)
   self._entity = entity
   self._destination_component = self._entity:add_component('destination')
   self._collision_shape_component = self._entity:add_component('region_collision_shape')

   self._sv = self.__saved_variables:get_data()
   if not self._sv.initialized then
      self._collision_shape_component
         :set_region_collision_type(_radiant.om.RegionCollisionShape.NONE)

      self._destination_component
         :set_region(_radiant.sim.alloc_region3())
         :set_adjacent(_radiant.sim.alloc_region3())
         :set_reserved(_radiant.sim.alloc_region3())

      self:set_region(_radiant.sim.alloc_region3())

      self._sv.mining_zone_enabled = true
      self._sv.initialized = true
      self.__saved_variables:mark_changed()
   else
      self:_restore()
   end

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
end

-- region is a boxed region
function MiningZoneComponent:get_region()
   return self._sv.region
end

-- region is a boxed region
function MiningZoneComponent:set_region(region)
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

function MiningZoneComponent:set_mining_zone_enabled(enabled)
   if self._sv.mining_zone_enabled == enabled then
      return
   end

   self._sv.mining_zone_enabled = enabled
   self:_update_mining_task()
end

function MiningZoneComponent:get_mining_zone_enabled()
   return self._sv.mining_zone_enabled
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
   -- cache the region bounds
   self._sv.region:modify(function(cursor)
         cursor:optimize_by_merge()
      end)
   self.__saved_variables:mark_changed()

   self:_update_destination()
   self:_create_mining_task()
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

local function count_open_faces_for_block(point, max)
   max = max or 2
   local count = 0

   for _, dir in ipairs(FACE_DIRECTIONS) do
      if not radiant.terrain.is_terrain(point + dir) then
         count = count + 1
         if count >= max then
            return count
         end
      end
   end

   return count
end

local function has_n_plus_exposed_faces(point, n)
   local count = count_open_faces_for_block(point, n)
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
         cursor:optimize_by_merge()
      end)

   self:_update_adjacent()
end

function MiningZoneComponent:_get_working_region(zone_cube, zone_location)
   local reserved_region = self._destination_component:get_reserved():get()
   local zone_region = Region3(zone_cube)
   local zone_reserved_region = zone_region:intersected(reserved_region)

   zone_region:subtract_region(zone_reserved_region)
   zone_region:translate(zone_location)
   zone_reserved_region:translate(zone_location)

   local working_region = radiant.terrain.intersect_region(zone_region)
   working_region:set_tag(0)
   working_region:optimize_by_merge()
   return working_region, zone_reserved_region
end

-- this algorithm assumes a convex region, so we break the zone into cubes before running it
-- working_region and zone_reserved_region are in world coordinates
-- destination_region and zone_cube are in local coordinates
function MiningZoneComponent:_add_destination_blocks(destination_region, zone_cube, zone_location)
   local world_to_local_translation = -zone_location
   local working_region, zone_reserved_region = self:_get_working_region(zone_cube, zone_location)
   local bounds = working_region:get_bounds()

   local add_block = function(point)
         local block = Cube3(point, point + Point3.one)
         block:translate(world_to_local_translation)
         destination_region:add_cube(block)
      end

   -- check top facing blocks
   for cube in working_region:each_cube() do
      if cube.max.y >= bounds.max.y - MAX_DESTINATION_DELTA_Y then
         local top_face = mining_lib.get_face(cube, Point3.unit_y)

         if top_face.max.y == bounds.max.y then
            for point in cube:each_point() do
               if face_is_exposed(point, Point3.unit_y) then
                  add_block(point)
               end
            end
         else
            for point in cube:each_point() do
               if face_is_exposed(point, Point3.unit_y) and not has_higher_neighbor(point, 1) then
                  add_block(point)
               end
            end
         end
      end
   end

   -- check side and bottom facing blocks
   for _, normal in ipairs(NON_TOP_DIRECTIONS) do
      local face_region = Region3(mining_lib.get_face(bounds, normal))
      local face_blocks = face_region:intersected(working_region)
      for point in face_blocks:each_point() do
         if face_is_exposed(point, normal) then
            add_block(point)
         end
      end
   end

   if destination_region:empty() then
      -- fallback condition
      log:warning('adding all exposed faces to destination')
      for cube in working_region:each_cube() do
         mining_lib.each_face_block_in_cube(cube, function(point)
               if has_n_plus_exposed_faces(point, 1) then
                  add_block(point)
               end
            end)
      end
   end

  if destination_region:empty() then
      -- fallback condition
      log:warning('adding all blocks to destination')
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

function MiningZoneComponent:_update_adjacent()
   local location = radiant.entities.get_world_grid_location(self._entity)
   if not location then
      return
   end

   local destination_region = self._destination_component:get_region():get()
   local reserved_region = self._destination_component:get_reserved():get()
   local unreserved_destination_region = destination_region - reserved_region

   self._destination_component:get_adjacent():modify(function(cursor)
         cursor:clear()

         for point in unreserved_destination_region:each_point() do
            local world_point = point + location
            local adjacent_region = stonehearth.mining:get_adjacent_for_destination_block(world_point)
            adjacent_region:translate(-location)
            cursor:add_region(adjacent_region)
         end

         cursor:optimize_by_merge()
      end)
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
   if self._sv.mining_zone_enabled then
      self:_create_mining_task()
   else
      self:_destroy_mining_task()
   end
end

return MiningZoneComponent
