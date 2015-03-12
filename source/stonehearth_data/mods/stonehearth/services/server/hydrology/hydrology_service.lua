local csg_lib = require 'lib.csg.csg_lib'
local ChannelManager = require 'services.server.hydrology.channel_manager'
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local log = radiant.log.create_logger('water')
local TraceCategories = _radiant.dm.TraceCategories

HydrologyService = class()

function HydrologyService:initialize()
   self._sv = self.__saved_variables:get_data()

   if not self._sv._initialized then
      self._sv._water_bodies = {}
      self._sv._channel_manager = radiant.create_controller('stonehearth:channel_manager')
      self._sv._initialized = true
      self.__saved_variables:mark_changed()
   else
   end

   if radiant.util.get_config('enable_water', false) then
      stonehearth.calendar:set_interval(10, function()
            self:_on_tick()
         end)
   end

   self:_trace_terrain_delta()
end

function HydrologyService:destroy()
   radiant.destroy_controller(self._sv._channel_manager)

   self:_destroy_terrain_delta_trace()
end

function HydrologyService:get_channel_manager()
   return self._sv._channel_manager
end

function HydrologyService:_trace_terrain_delta()
   local terrain_component = radiant._root_entity:add_component('terrain')
   self._water_tight_region = terrain_component:get_water_tight_region()
   self._delta_trace = terrain_component:trace_water_tight_region_delta('hydrology service', TraceCategories.SYNC_TRACE)
      :on_changed(function(delta_region)
            self:_on_terrain_changed(delta_region)
         end)
      :push_object_state()
end

function HydrologyService:_destroy_terrain_delta_trace()
   if self._delta_trace then
      self._delta_trace:destroy()
      self._delta_trace = nil
   end
end

function HydrologyService:_on_terrain_changed(delta_region)
   local inflated_delta_bounds = self:_get_inflated_delta_bounds(delta_region)
   if not inflated_delta_bounds then
      return
   end

   local channel_manager = self:get_channel_manager()

   for id, entity in pairs(self._sv._water_bodies) do
      -- fast-ish rejection test to see if the delta region modifies the water region or its container
      if self:_bounds_intersects_water_body(inflated_delta_bounds, entity) then
         local modified_water_region = self:_get_affected_water_region(delta_region, entity)
         for point in modified_water_region:each_point() do
            if self._water_tight_region:contains_point(point) then
               -- TODO: merge may have occured
               -- TODO: check for unused volume
               -- TODO: check for bisection at end
               -- TODO: if region is empty, delete water body
               self:add_water(1, point, entity)
            end
         end

         local modified_container_region = self:_get_affected_container_region(delta_region, entity)
         for point in modified_container_region:each_point() do
            if not self._water_tight_region:contains_point(point) then
               -- TODO: no need to create channel if top layer height is 0
               local target_entity = self:create_water_body(point)
               local target_adjacent_point = self:_get_best_channel_adjacent_point(entity, point)
               local channel

               if point.y == target_adjacent_point.y then
                  channel = channel_manager:link_pressure_channel(entity, point, target_entity, target_adjacent_point)
               else
                  channel = channel_manager:link_waterfall_channel(entity, point)
               end

               -- TODO: we should probably add_volume_to_channel here
            end
         end

         -- TODO: remove channels that are no longer adjacent to the entity
      end
   end
end

-- can be faster if necessary
function HydrologyService:_get_best_channel_adjacent_point(entity, point)
   local entity_location = radiant.entities.get_world_grid_location(entity)
   local water_component = entity:add_component('stonehearth:water')
   local water_region = water_component:get_region():get()

   local point_region = Region3()
   point_region:add_point(point - entity_location)
   local adjacent = nil

   local inflated = csg_lib.get_non_diagonal_xz_inflated_region(point_region)
   local intersection = inflated:intersect_region(water_region)
   if not intersection:empty() then
      adjacent = intersection:get_closest_point(point)
   end

   if not adjacent then
      inflated = point_region:inflated(Point3.unit_y)
      intersection = inflated:intersect_region(water_region)
      if not intersection:empty() then
         adjacent = intersection:get_closest_point(point)
      end
   end

   if not adjacent then
      -- point is not adjacent to entity
      assert(false)
   end

   local world_adjacent = adjacent + entity_location
   return world_adjacent
end

-- bounds are in world coordinates
function HydrologyService:_bounds_intersects_water_body(inflated_delta_bounds, entity)
   local entity_location = radiant.entities.get_world_grid_location(entity)
   local local_inflated_delta_bounds = inflated_delta_bounds:translated(-entity_location)

   local water_component = entity:add_component('stonehearth:water')
   local water_region = water_component:get_region():get()
   local result = water_region:intersects_cube(local_inflated_delta_bounds)
   return result
end

-- returns the intersection of the delta region and the water body proper (and not its container)
-- delta region and result are in world coordinates
function HydrologyService:_get_affected_water_region(delta_region, entity)
   local entity_location = radiant.entities.get_world_grid_location(entity)
   local local_delta_region = delta_region:translated(-entity_location)

   local water_component = entity:add_component('stonehearth:water')
   local water_region = water_component:get_region():get()
   local intersection = water_region:intersect_region(local_delta_region)
   intersection:translate(entity_location)
   return intersection
end

-- returns the intersection of the delta region and the side and bottom walls of the water body's container
-- delta region and result are in world coordinates
function HydrologyService:_get_affected_container_region(delta_region, entity)
   local entity_location = radiant.entities.get_world_grid_location(entity)
   local local_delta_region = delta_region:translated(-entity_location)

   local container_region = self:_get_container_region(entity)
   local intersection = container_region:intersect_region(local_delta_region)
   intersection:translate(entity_location)
   return intersection
end

function HydrologyService:_get_inflated_delta_bounds(region)
   if region:empty() then
      return nil
   end

   local bounds = region:get_bounds()
   local lower_bound = bounds.min.y
   bounds = bounds:inflated(Point3.one)
   -- don't inflate the bottom because water isn't affected by blocks above the surface
   bounds.min.y = lower_bound
   return bounds
end

-- returns the watertight region that contains the water region in local coordinates
function HydrologyService:_get_container_region(entity)
   local water_component = entity:add_component('stonehearth:water')
   local water_region = water_component:get_region():get()
   local container_region = Region3()

   -- add the walls to the container
   container_region:add_region(water_region:inflated(Point3.unit_x))
   container_region:add_region(water_region:inflated(Point3.unit_z))

   -- add the floor to the container
   local floor_region = Region3()
   for cube in water_region:each_cube() do
      local inflated_cube = Cube3(cube)
      inflated_cube.min.y = inflated_cube.min.y - 1
      floor_region:add_cube(inflated_cube)
   end
   container_region:add_region(floor_region)

   -- subtract the water body to get the container
   container_region:subtract_region(water_region)

   return container_region
end

function HydrologyService:create_water_body(location)
   local entity = radiant.entities.create_entity('stonehearth:terrain:water')
   log:debug('Creating water body %s at %s', entity, location)

   local id = entity:get_id()
   self._sv._water_bodies[id] = entity
   self._sv._channel_manager:allocate_channels(entity)
   radiant.terrain.place_entity_at_exact_location(entity, location)
   self.__saved_variables:mark_changed()

   return entity
end

function HydrologyService:get_water_body(location)
   for id, entity in pairs(self._sv._water_bodies) do
      local entity_location = radiant.entities.get_world_grid_location(entity)
      local local_location = location - entity_location
      local water_component = entity:add_component('stonehearth:water')
      local region = water_component:get_region():get()

      -- cache the region bounds as a quick rejection test if this becomes slow
      if region:contains(local_location) then
         return entity
      end
   end

   return nil
end

function HydrologyService:remove_water_body(entity)
   log:debug('Removing water body %s', entity)

   local id = entity:get_id()
   self._sv._water_bodies[id] = nil
   self._sv._channel_manager:deallocate_channels(entity)
   self.__saved_variables:mark_changed()
end

-- entity is an optional hint
-- returns volume of water that could not be added
function HydrologyService:add_water(volume, location, entity)
   if not entity then
      entity = self:get_water_body(location)
   end

   local water_component = entity:add_component('stonehearth:water')
   local volume, info = water_component:_add_water(location, volume)

   if volume > 0 and info then
      assert(info.result == 'merge')
      -- the entity lower in elevation is the master
      -- mergee is destroyed in this call
      local master = self:merge_water_bodies(info.entity1, info.entity2)
      -- add the remaining water to the master
      return self:add_water(volume, location, master)
   end

   return volume
end

-- must specify either location or entity
-- returns volume of water that could not be removed
function HydrologyService:remove_water(volume, location, entity)
   if not entity then
      entity = self:get_water_body(location)
   end

   local water_component = entity:add_component('stonehearth:water')
   local volume = water_component:_remove_water(volume, location, entity)

   return volume
end

function HydrologyService:can_merge_water_bodies(entity1, entity2)
   assert(entity1 ~= entity2)
   local water_component1 = entity1:add_component('stonehearth:water')
   local water_component2 = entity2:add_component('stonehearth:water')
   local water_level1 = water_component1:get_water_level()
   local water_level2 = water_component2:get_water_level()
   local elevation_delta = math.abs(water_level1 - water_level2)

   -- quick and easy test. occurs a lot when merging wetted regions.
   if elevation_delta == 0 then
      return true
   end

   -- only mergable if the top layers are at the same elevation
   local layer_elevation1 = water_component1:get_current_layer_elevation()
   local layer_elevation2 = water_component2:get_current_layer_elevation()
   if layer_elevation1 ~= layer_elevation2 then
      return false
   end

   -- if transferring a small volume of water through the channel would equalize heights, then allow merge
   -- TODO: consider optimizing repeated calls to O(n) get_area()
   local merge_elevation_threshold = 0.10
   if elevation_delta < merge_elevation_threshold then
      local merge_volume_threshold = 1
      local entity1_layer_area = water_component1._sv._current_layer:get():get_area()
      local entity2_layer_area = water_component2._sv._current_layer:get():get_area()
      local entity1_delta_height = merge_volume_threshold / entity1_layer_area
      local entity2_delta_height = merge_volume_threshold / entity2_layer_area
      if entity1_delta_height + entity2_delta_height >= elevation_delta then
         assert(elevation_delta < 1)
         return true
      end
   end

   return false
end

function HydrologyService:merge_water_bodies(entity1, entity2)
   assert(entity1 ~= entity2)
   local master, mergee = self:_order_entities(entity1, entity2)
   self:_merge_water_bodies_impl(master, mergee)
   return master
end

-- orders entities by increasing elevation
function HydrologyService:_order_entities(entity1, entity2)
   local entity1_elevation = radiant.entities.get_world_grid_location(entity1).y
   local entity2_elevation = radiant.entities.get_world_grid_location(entity2).y

   if entity1_elevation == entity2_elevation then
      -- if at the same elevation prefer the larger water body
      local volume1 = entity1:add_component('stonehearth:water'):get_region():get():get_area()
      local volume2 = entity2:add_component('stonehearth:water'):get_region():get():get_area()
      if volume1 > volume2 then
         return entity1, entity2
      else
         return entity2, entity1
      end
   end

   if entity1_elevation < entity2_elevation then
      return entity1, entity2
   else
      return entity2, entity1
   end
end

function HydrologyService:_merge_water_bodies_impl(master, mergee)
   assert(master ~= mergee)
   local master_location = radiant.entities.get_world_grid_location(master)
   local mergee_location = radiant.entities.get_world_grid_location(mergee)
   log:debug('Merging %s at %s with %s at %s', master, master_location, mergee, mergee_location)

   self:_merge_regions(master, mergee)
   self:_merge_water_queue(master, mergee)
   self._sv._channel_manager:merge_channels(master, mergee)

   self:remove_water_body(mergee)
   radiant.entities.destroy_entity(mergee)

   self.__saved_variables:mark_changed()
end

function HydrologyService:_merge_regions(master, mergee)
   local master_location = radiant.entities.get_world_grid_location(master)
   local mergee_location = radiant.entities.get_world_grid_location(mergee)
   local master_component = master:add_component('stonehearth:water')
   local mergee_component = mergee:add_component('stonehearth:water')

   -- layers must be at same level
   local master_layer_elevation = master_component:get_current_layer_elevation()
   local mergee_layer_elevation = mergee_component:get_current_layer_elevation()
   assert(master_layer_elevation == mergee_layer_elevation)

   -- get the heights of the current layers
   local master_layer_height = master_component:get_water_level() - master_layer_elevation
   local mergee_layer_height = mergee_component:get_water_level() - mergee_layer_elevation
   assert(master_layer_height <= 1)
   assert(mergee_layer_height <= 1)

   -- calculate the merged layer height
   local master_layer_area = master_component._sv._current_layer:get():get_area()
   local mergee_layer_area = mergee_component._sv._current_layer:get():get_area()
   local new_layer_area = master_layer_area + mergee_layer_area
   local new_layer_volume = master_layer_area * master_layer_height + mergee_layer_area * mergee_layer_height
   local new_layer_height = new_layer_volume / new_layer_area
   local new_height = master_layer_elevation + new_layer_height - master_location.y

   -- translate between local coordinate systems
   local translation = mergee_location - master_location

   -- merge the regions
   master_component._sv.region:modify(function(cursor)
         local mergee_region = mergee_component._sv.region:get():translated(translation)
         cursor:add_region(mergee_region)
      end)

   -- merge the working layers
   master_component._sv._current_layer:modify(function(cursor)
         local mergee_layer = mergee_component._sv._current_layer:get():translated(translation)
         cursor:add_region(mergee_layer)
      end)

   master_component._sv.height = new_height

   -- clear the mergee regions
   mergee_component._sv.region:modify(function(cursor)
         cursor:clear()
      end)

   mergee_component._sv._current_layer:modify(function(cursor)
         cursor:clear()
      end)
end

function HydrologyService:_merge_water_queue(master, mergee)
   if not self._water_queue then
      return
   end

   for _, entry in ipairs(self._water_queue) do
      -- redirect all mergee references to master
      if entry.entity == mergee then
         entry.entity = master
      end
   end
end

function HydrologyService:_on_tick()
   log:spam('Start tick')

   self:_update_performance_counters()

   self._sv._channel_manager:fill_channels_to_capacity()

   -- TODO: fix the channel rendering lagging the volume by 1 tick
   self._sv._channel_manager:update_channel_entities()

   log:spam('Emptying channels into water queue')
   self._water_queue = self._sv._channel_manager:empty_channels()

   -- check when the channels are empty to avoid destroying channels with queued water
   self:_check_for_channel_merge()
   self._sv._channel_manager:update_channel_types()

   for i, entry in ipairs(self._water_queue) do
      local unused_volume = self:add_water(entry.volume, entry.location, entry.entity)
      -- TODO: we currently lose the water if the water body cannot grow (e.g. punch a hole in the wall below water level)
      -- maybe add it back to where it came from
   end

   self._water_queue = nil

   log:spam('End tick')

   self.__saved_variables:mark_changed()
end

function HydrologyService:_check_for_channel_merge()
   -- TODO: can we clean this up?
   repeat
      local restart = false
      self._sv._channel_manager:each_channel(function(channel)
            if self:can_merge_water_bodies(channel.from_entity, channel.to_entity) then
               self:merge_water_bodies(channel.from_entity, channel.to_entity)
               restart = true
               return true -- stop iteration
            end
            return nil
         end)
   until not restart
end

function HydrologyService:_update_performance_counters()
   local num_water_bodies = 0
   for id, entity in pairs(self._sv._water_bodies) do
      num_water_bodies = num_water_bodies + 1
   end
   radiant.set_performance_counter('num_water_bodies', num_water_bodies)

   local num_channels = 0
   self._sv._channel_manager:each_channel(function(channel)
         num_channels = num_channels + 1
      end)
   radiant.set_performance_counter('num_channels', num_channels)
end

return HydrologyService
