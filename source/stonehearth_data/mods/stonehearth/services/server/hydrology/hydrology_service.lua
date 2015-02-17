local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local log = radiant.log.create_logger('water')

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
end

function HydrologyService:destroy()
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
      local master, mergee = self:_order_entities(info.entity1, info.entity2)
      -- mergee is destroyed in this call
      self:merge_water_bodies(master, mergee)
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
      local waterfall = self:_create_waterfall(from_location, to_location)
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
         callback_fn(channel)
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

function HydrologyService:_create_waterfall(from_location, to_location)
   local entity = radiant.entities.create_entity('stonehearth:terrain:waterfall')
   radiant.terrain.place_entity_at_exact_location(entity, from_location)
   local waterfall_component = entity:add_component('stonehearth:waterfall')
   waterfall_component:set_height(from_location.y - to_location.y)
   return entity
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

function HydrologyService:merge_water_bodies(master, mergee)
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

   -- hydrostatic pressures must be equal in order to merge
   assert(master_component:get_water_elevation() == mergee_component:get_water_elevation())

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
   self:_each_channel(function(channel)
         -- all channels originating from mergee now originate from master
         if channel.from_entity and channel.from_entity == mergee then
            channel.from_entity = master
         end

         -- all channels going to mergee now go to master
         if channel.to_entity == mergee then
            channel.to_entity = master
         end

         if channel.from_entity == channel.to_entity then
            -- channel now goes to itself, so just destroy it
            self:_destroy_channel(channel)
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

   for i, entry in ipairs(self._water_queue) do
      self:add_water(entry.volume, entry.location, entry.entity)
   end

   self._water_queue = nil

   log:spam('End tick')

   self.__saved_variables:mark_changed()
end

-- orders entities by increasing elevation
function HydrologyService:_order_entities(entity1, entity2)
   local entity1_location = radiant.entities.get_world_grid_location(entity1)
   local entity2_location = radiant.entities.get_world_grid_location(entity2)

   if entity1_location.y <= entity2_location.y then
      return entity1, entity2
   else
      return entity2, entity1
   end
end

function HydrologyService:_empty_channels()
   local water_queue = {}

   self:_each_channel(function(channel)
         local entry = self:_create_water_queue_entry(channel.to_entity, channel.to_location, channel.queued_volume)
         table.insert(water_queue, entry)

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
