local csg_lib = require 'lib.csg.csg_lib'
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local log = radiant.log.create_logger('water')
local TraceCategories = _radiant.dm.TraceCategories

HydrologyService = class()

function HydrologyService:initialize()
   self._sv = self.__saved_variables:get_data()

   -- constant converting pressure to a flow rate per unit cross section
   self._pressure_to_flow_rate = 1

   if not self._sv._initialized then
      self._sv._water_bodies = {}
      self._sv._channels = {}
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
   self:_destroy_terrain_delta_trace()
end

function HydrologyService:_trace_terrain_delta()
   local terrain_component = radiant._root_entity:add_component('terrain')
   self._delta_trace = terrain_component:trace_delta_region('hydrology service', TraceCategories.SYNC_TRACE)
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

   for id, entity in pairs(self._sv._water_bodies) do
      -- fast-ish rejection test to see if the delta region modifies the water region or its container
      if self:_bounds_intersects_water_body(inflated_delta_bounds, entity) then
         local modified_water_region = self:_get_affected_water_region(delta_region, entity)
         for point in modified_water_region:each_point() do
            if radiant.terrain.is_terrain(point) then
               -- TODO: merge may have occured
               -- TODO: check for unused volume
               -- TODO: check for bisection at end
               -- TODO: if region is empty, delete water body
               self:add_water(1, point, entity)
            end
         end

         local modified_container_region = self:_get_affected_container_region(delta_region, entity)
         for point in modified_container_region:each_point() do
            if not radiant.terrain.is_terrain(point) then
               -- TODO: handle vertical case
               local target_entity = self:create_water_body(point)
               local target_adjacent_point = self:_get_closest_point_to(point, entity)
               stonehearth.hydrology:link_pressure_channel(entity, point, target_entity, target_adjacent_point)
            end
         end
      end
   end
end

-- point and result are in world coordinates
function HydrologyService:_get_closest_point_to(world_point, entity)
   local water_component = entity:add_component('stonehearth:water')
   local entity_location = radiant.entities.get_world_grid_location(entity)
   local local_point = world_point - entity_location
   local water_region = water_component:get_region():get()
   local local_closest_point = water_region:get_closest_point(local_point)
   local closest_point = local_closest_point:translated(entity_location)
   return closest_point
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
   self._sv._channels[id] = {}
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
   self._sv._channels[id] = nil
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

   if volume > 0 then
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

function HydrologyService:link_waterfall_channel(from_entity, from_location)
   local channel = stonehearth.hydrology:get_channel(from_entity, from_location)

   if channel and channel.channel_type ~= 'waterfall' then
      stonehearth.hydrology:remove_channel(from_entity, from_location)
      channel = nil
   end

   if not channel then
      local to_entity, to_location = self:_get_waterfall_target(from_location)
      local waterfall = self:_create_waterfall(from_entity, from_location, to_entity, to_location)
      channel = self:_add_channel(from_entity, from_location, to_entity, to_location, 'waterfall', waterfall)
   end
   return channel
end

function HydrologyService:_get_waterfall_target(from_location)
   local to_location = radiant.terrain.get_point_on_terrain(from_location)
   local to_entity = self:get_water_body(to_location)
   if not to_entity then
      to_entity = self:create_water_body(to_location)
   end
   return to_entity, to_location
end

-- Confusing, but the source_adjacent_point is inside the target and the target_adjacent_point
-- is inside the source. This is because the from_location of the channel is defined to be outside
-- the region of the source water body.
function HydrologyService:link_pressure_channel(source_entity, source_adjacent_point, target_entity, target_adjacent_point)
   local forward_channel = self:_link_pressure_channel_unidirectional(source_entity, source_adjacent_point,
                                                                      target_entity, source_adjacent_point)

   local reverse_channel = self:_link_pressure_channel_unidirectional(target_entity, target_adjacent_point,
                                                                      source_entity, target_adjacent_point)

   return forward_channel
end

function HydrologyService:_link_pressure_channel_unidirectional(from_entity, from_location, to_entity, to_location)
   -- note that the to_location is the same as the from_location
   local channel = stonehearth.hydrology:get_channel(from_entity, from_location)

   if channel and channel.channel_type ~= 'pressure' then
      stonehearth.hydrology:remove_channel(from_entity, from_location)
      channel = nil
   end

   if not channel then
      channel = self:_add_channel(from_entity, from_location, to_entity, to_location, 'pressure', nil)
   end
   return channel
end

function HydrologyService:get_channel(from_entity, from_location)
   local channels = self:get_channels(from_entity)
   local key = self:_point_to_key(from_location)
   local channel = channels[key]   
   return channel
end

function HydrologyService:get_channels(from_entity)
   local channels = self._sv._channels[from_entity:get_id()]
   return channels
end

function HydrologyService:remove_channel(from_entity, from_location)
   local channels = self:get_channels(from_entity)
   local key = self:_point_to_key(from_location)
   local channel = channels[key]

   log:debug('Removing %s channel from %s at %s', channel.channel_type, from_entity, from_location)

   if not channel then
      return
   end

   channels[key] = nil
   self:_destroy_channel(channel)

   self.__saved_variables:mark_changed()
end

function HydrologyService:_destroy_channel(channel)
   if channel.channel_entity then
      radiant.entities.destroy_entity(channel.channel_entity)
   end
end

function HydrologyService:_each_channel(callback_fn)
   for id, channels in pairs(self._sv._channels) do
      for _, channel in pairs(channels) do
         local stop = callback_fn(channel)
         if stop then
            return
         end
      end
   end
end

function HydrologyService:_add_channel(from_entity, from_location, to_entity, to_location, channel_type, channel_entity)
   log:debug('Adding %s channel from %s at %s to %s at %s', channel_type, from_entity, from_location, to_entity, to_location)

   local channel = self:_create_channel(from_entity, from_location, to_entity, to_location, channel_type, channel_entity)
   local key = self:_point_to_key(from_location)
   local channels = self:get_channels(from_entity)
   channels[key] = channel

   self.__saved_variables:mark_changed()
   return channel
end

function HydrologyService:_create_waterfall(from_entity, from_location, to_entity, to_location)
   local waterfall = radiant.entities.create_entity('stonehearth:terrain:waterfall')
   radiant.terrain.place_entity_at_exact_location(waterfall, from_location)
   local waterfall_component = waterfall:add_component('stonehearth:waterfall')
   waterfall_component:set_height(from_location.y - to_location.y)
   waterfall_component:set_source(from_entity)
   waterfall_component:set_target(to_entity)
   return waterfall
end

function HydrologyService:calculate_channel_flow_rate(channel)
   if channel.channel_type == 'waterfall' then
      local water_component = channel.from_entity:add_component('stonehearth:water')
      local water_elevation = water_component:get_water_elevation()
      local target_elevation = channel.from_location.y - 1
      local flow_rate = self:calculate_flow_rate(water_elevation, target_elevation)
      return flow_rate
   end

   if channel.channel_type == 'pressure' then
      local from_water_component = channel.from_entity:add_component('stonehearth:water')
      local to_water_component = channel.to_entity:add_component('stonehearth:water')
      local from_water_elevation = from_water_component:get_water_elevation()
      local to_water_elevation = to_water_component:get_water_elevation()
      local flow_rate = self:calculate_flow_rate(from_water_elevation, to_water_elevation)
      return flow_rate
   end
end

-- returns flow rate in voxels / tick / unit square
-- flow rate may be negative
function HydrologyService:calculate_flow_rate(from_elevation, to_elevation)
   local pressure = from_elevation - to_elevation
   local flow_rate = pressure * self._pressure_to_flow_rate
   return flow_rate
end

function HydrologyService:can_merge_water_bodies(entity1, entity2)
   assert(entity1 ~= entity2)
   local water_component1 = entity1:add_component('stonehearth:water')
   local water_component2 = entity2:add_component('stonehearth:water')
   local water_elevation1 = water_component1:get_water_elevation()
   local water_elevation2 = water_component2:get_water_elevation()
   local elevation_delta = math.abs(water_elevation1 - water_elevation2)

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
   local merge_volume_threshold = 1
   local entity1_layer_area = water_component1._sv._current_layer:get():get_area()
   local entity2_layer_area = water_component2._sv._current_layer:get():get_area()
   local entity1_delta_height = merge_volume_threshold / entity1_layer_area
   local entity2_delta_height = merge_volume_threshold / entity2_layer_area
   if entity1_delta_height + entity2_delta_height >= elevation_delta then
      assert(elevation_delta < 1)
      return true
   end

   return false
end

function HydrologyService:merge_water_bodies(entity1, entity2)
   assert(entity1 ~= entity2)
   local master, mergee = self:_order_entities(entity1, entity2)
   self:_merge_water_bodies_impl(entity1, entity2)
   return master
end

function HydrologyService:_merge_water_bodies_impl(master, mergee)
   assert(master ~= mergee)
   local master_location = radiant.entities.get_world_grid_location(master)
   local mergee_location = radiant.entities.get_world_grid_location(mergee)
   log:debug('Merging %s at %s with %s at %s', master, master_location, mergee, mergee_location)

   self:_merge_regions(master, mergee)
   self:_merge_channels(master, mergee)
   self:_merge_water_queue(master, mergee)

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
   assert(master_component:get_current_layer_elevation() == mergee_component:get_current_layer_elevation())

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

   -- clear the mergee regions
   mergee_component._sv.region:modify(function(cursor)
         cursor:clear()
      end)

   mergee_component._sv._current_layer:modify(function(cursor)
         cursor:clear()
      end)
end

function HydrologyService:_merge_channels(master, mergee)
   local master_channels = self:get_channels(master)
   local mergee_channels = self:get_channels(mergee)

   -- reparent all mergee channels to master
   for key, channel in pairs(mergee_channels) do
      assert(channel.from_entity == mergee)

      channel.from_entity = master
      master_channels[key] = channel
      mergee_channels[key] = nil
   end

   -- redirect all channels from mergee to master
   self:_each_channel(function(channel)
      assert(channel.from_entity ~= mergee)

      if channel.to_entity == mergee then
         channel.to_entity = master
      end

      if channel.from_entity == channel.to_entity then
         -- channel now goes to itself, so destroy it
         self:remove_channel(channel.from_entity, channel.from_location)
      end
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

   self:_each_channel(function(channel)
         local water_component = channel.from_entity:add_component('stonehearth:water')
         water_component:_fill_channels_to_capacity()
      end)

   -- TODO: fix the channel rendering lagging the volume by 1 tick
   self:_update_channel_entities()

   log:spam('Emptying channels into water queue')
   self._water_queue = self:_empty_channels()

   -- check when the channels are empty to avoid destroying channels with queued water
   self:_check_for_channel_merge()

   for i, entry in ipairs(self._water_queue) do
      self:add_water(entry.volume, entry.location, entry.entity)
   end

   self._water_queue = nil

   log:spam('End tick')

   self.__saved_variables:mark_changed()
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

function HydrologyService:_check_for_channel_merge()
   repeat
      local restart = false
      self:_each_channel(function(channel)
            if self:can_merge_water_bodies(channel.from_entity, channel.to_entity) then
               self:merge_water_bodies(channel.from_entity, channel.to_entity)
               restart = true
               return true -- stop iteration
            end
            return nil
         end)
   until not restart
end

function HydrologyService:_empty_channels()
   local water_queue = {}

   self:_each_channel(function(channel)
         if channel.queued_volume > 0 then
            local entry = self:_create_water_queue_entry(channel.to_entity, channel.to_location, channel.queued_volume)
            table.insert(water_queue, entry)
         end

         -- empty the channel so we can queue for the next iteration
         channel.queued_volume = 0
      end)

   self.__saved_variables:mark_changed()

   return water_queue
end

function HydrologyService:_create_water_queue_entry(entity, location, volume)
   local entry = {
      entity = entity,
      location = location,
      volume = volume
   }
   return entry
end

function HydrologyService:_update_channel_entities()
   self:_each_channel(function(channel)
         if channel.channel_entity then
            local waterfall_component = channel.channel_entity:get_component('stonehearth:waterfall')
            if waterfall_component then
               waterfall_component:set_volume(channel.queued_volume)
            end
         end
      end)
end

function HydrologyService:_create_channel(from_entity, from_location, to_entity, to_location, channel_type, channel_entity)
   local channel = {
      from_entity = from_entity,
      from_location = from_location,
      to_entity = to_entity,
      to_location = to_location,
      channel_type = channel_type,
      channel_entity = channel_entity,
      queued_volume = 0
   }

   self.__saved_variables:mark_changed()

   return channel
end

function HydrologyService:_point_to_key(point)
   return string.format('%d,%d,%d', point.x, point.y, point.z)
end

return HydrologyService
