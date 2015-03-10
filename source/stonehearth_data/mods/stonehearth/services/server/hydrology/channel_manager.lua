local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local log = radiant.log.create_logger('water')

local ChannelManager = class()

-- constant converting pressure to a flow rate per unit cross section
local PRESSURE_TO_FLOW_RATE = 1

-- a 'drop' of water
local MIN_FLOW_RATE = 0.01

function ChannelManager:initialize()
   assert(self._sv)
   self._sv._channels = {}
   self.__saved_variables:mark_changed()
end

function ChannelManager:allocate_channels(from_entity)
   local channels = self:get_channels(from_entity)
   if not channels then
      self._sv._channels[from_entity:get_id()] = {}
      self.__saved_variables:mark_changed()
   end
end

function ChannelManager:deallocate_channels(from_entity)
   self._sv._channels[from_entity:get_id()] = nil
   self.__saved_variables:mark_changed()
end

function ChannelManager:get_channel(from_entity, from_location)
   local channels = self:get_channels(from_entity)
   local key = self:_point_to_key(from_location)
   local channel = channels[key]   
   return channel
end

function ChannelManager:get_channels(from_entity)
   local channels = self._sv._channels[from_entity:get_id()]
   return channels
end

function ChannelManager:add_channel(from_entity, from_location, to_entity, to_location, channel_type, channel_entity)
   log:debug('Adding %s channel from %s at %s to %s at %s', channel_type, from_entity, from_location, to_entity, to_location)

   local channel = self:_create_channel(from_entity, from_location, to_entity, to_location, channel_type, channel_entity)
   local key = self:_point_to_key(from_location)
   local channels = self:get_channels(from_entity)
   channels[key] = channel

   self.__saved_variables:mark_changed()
   return channel
end

function ChannelManager:_create_channel(from_entity, from_location, to_entity, to_location, channel_type, channel_entity)
   local channel = {
      from_entity = from_entity,
      from_location = from_location,
      to_entity = to_entity,
      to_location = to_location,
      channel_type = channel_type,
      channel_entity = channel_entity,
      queued_volume = 0
   }

   return channel
end

function ChannelManager:remove_channel(channel)
   local paired_channel = channel.paired_channel
   self:_remove_unidirectional_channel(channel)

   if paired_channel then
      self:_remove_unidirectional_channel(paired_channel)
   end
end

function ChannelManager:_remove_unidirectional_channel(channel)
   local channels = self:get_channels(channel.from_entity)
   local key = self:_point_to_key(channel.from_location)

   log:debug('Removing %s channel from %s at %s', channel.channel_type, channel.from_entity, channel.from_location)

   channels[key] = nil
   self:_destroy_channel(channel)

   self.__saved_variables:mark_changed()
end

function ChannelManager:_destroy_channel(channel)
   if channel.channel_entity then
      radiant.entities.destroy_entity(channel.channel_entity)
   end
end

function ChannelManager:each_channel(callback_fn)
   for id, channels in pairs(self._sv._channels) do
      for _, channel in pairs(channels) do
         local stop = callback_fn(channel)
         if stop then
            return
         end
      end
   end
end

function ChannelManager:each_channel_ascending(callback_fn)
   local channels = {}
   self:each_channel(function(channel)
         table.insert(channels, channel)
      end)

   local sorted_channels = self:sort_channels_ascending(channels)
   for _, channel in ipairs(sorted_channels) do
      callback_fn(channel)
   end
end

function ChannelManager:add_volume_to_channel(channel, volume, source_elevation_bias)
   assert(volume >= 0)

   -- get the flow volume per tick
   local max_flow_volume = self:calculate_channel_flow_rate(channel, source_elevation_bias)

   if max_flow_volume <= 0 then
      -- not enough pressure to add water to channel
      -- the channel in the reverse direction may flow this way however
      return volume
   end

   local unused_volume = max_flow_volume - channel.queued_volume
   local flow_volume = math.min(unused_volume, volume)
   channel.queued_volume = channel.queued_volume + flow_volume
   volume = volume - flow_volume

   if flow_volume > 0 then
      log:spam('Added %d to channel for %s at %s', flow_volume, channel.from_entity, channel.from_location)
   end

   return volume
end

-- source elevation bias is used when we want to compute the flow rate for a packet of water
-- that moves directly into the channel and skips being part of the source height
function ChannelManager:calculate_channel_flow_rate(channel, source_elevation_bias)
   source_elevation_bias = source_elevation_bias or 0

   local from_water_component = channel.from_entity:add_component('stonehearth:water')
   local source_elevation = from_water_component:get_water_level() + source_elevation_bias
   local target_elevation

   if channel.channel_type == 'waterfall' then
      target_elevation = channel.from_location.y
   elseif channel.channel_type == 'pressure' then
      local to_water_component = channel.to_entity:add_component('stonehearth:water')
      target_elevation = to_water_component:get_water_level()
   else
      assert(false)
   end

   local flow_rate = self:calculate_flow_rate(source_elevation, target_elevation)
   return flow_rate
end

-- returns flow rate in voxels / tick / unit square
-- flow rate may be negative
function ChannelManager:calculate_flow_rate(from_elevation, to_elevation)
   local pressure = from_elevation - to_elevation
   local flow_rate = pressure * PRESSURE_TO_FLOW_RATE

   -- stop flowing when less than a "drop" of water
   -- we don't want to keep computing immaterial deltas
   if flow_rate < MIN_FLOW_RATE then
      flow_rate = 0
   end

   return flow_rate
end

function ChannelManager:link_waterfall_channel(from_entity, from_location)
   local channel = self:get_channel(from_entity, from_location)

   if channel and channel.channel_type ~= 'waterfall' then
      assert(channel.channel_type == 'pressure')
      -- this removes both directions of the pressure channel
      self:remove_channel(channel)
      channel = nil
   end

   if not channel then
      local to_entity, to_location = self:_get_waterfall_target(from_location)
      local waterfall = self:_create_waterfall(from_entity, from_location, to_entity, to_location)
      channel = self:add_channel(from_entity, from_location, to_entity, to_location, 'waterfall', waterfall)
   end
   return channel
end

-- Confusing, but the source_adjacent_point is inside the target and the target_adjacent_point
-- is inside the source. This is because the from_location of the channel is defined to be outside
-- the region of the source water body.
function ChannelManager:link_pressure_channel(source_entity, source_adjacent_point, target_entity, target_adjacent_point)
   local forward_channel = self:_link_pressure_channel_unidirectional(source_entity, source_adjacent_point,
                                                                             target_entity, source_adjacent_point)

   local reverse_channel = self:_link_pressure_channel_unidirectional(target_entity, target_adjacent_point,
                                                                             source_entity, target_adjacent_point)

   forward_channel.paired_channel = reverse_channel
   reverse_channel.paired_channel = forward_channel

   return forward_channel
end

function ChannelManager:_link_pressure_channel_unidirectional(from_entity, from_location, to_entity, to_location)
   -- note that the to_location is the same as the from_location
   local channel = self:get_channel(from_entity, from_location)

   if channel and channel.channel_type ~= 'pressure' then
      self:remove_channel(channel)
      channel = nil
   end

   if not channel then
      channel = self:add_channel(from_entity, from_location, to_entity, to_location, 'pressure', nil)
   end
   return channel
end

function ChannelManager:_get_waterfall_target(from_location)
   local to_location = radiant.terrain.get_point_on_terrain(from_location)
   local to_entity = stonehearth.hydrology:get_water_body(to_location)
   if not to_entity then
      to_entity = stonehearth.hydrology:create_water_body(to_location)
   end
   return to_entity, to_location
end

function ChannelManager:_create_waterfall(from_entity, from_location, to_entity, to_location)
   local waterfall = radiant.entities.create_entity('stonehearth:terrain:waterfall')
   radiant.terrain.place_entity_at_exact_location(waterfall, from_location)
   local waterfall_component = waterfall:add_component('stonehearth:waterfall')
   waterfall_component:set_height(from_location.y - to_location.y)
   waterfall_component:set_source(from_entity)
   waterfall_component:set_target(to_entity)
   return waterfall
end

function ChannelManager:merge_channels(master, mergee)
   local master_channels = self:get_channels(master)
   local mergee_channels = self:get_channels(mergee)

   -- reparent all mergee channels to master
   self:reparent_channels(mergee_channels, master)

   -- redirect all channels from mergee to master
   self:each_channel(function(channel)
      assert(channel.from_entity ~= mergee)

      if channel.to_entity == mergee then
         channel.to_entity = master
      end

      if channel.from_entity == channel.to_entity then
         -- channel now goes to itself, so destroy it
         self:remove_channel(channel)
      end
   end)
end

function ChannelManager:reparent_channels(channels, new_parent)
   local new_parent_channels = self:get_channels(new_parent)

   for _, channel in pairs(channels) do
      if channel.to_entity == new_parent then
         -- channel would go to itself, so just remove it
         self:remove_channel(channel)
      else
         local old_parent = channel.from_entity
         channel.from_entity = new_parent

         local key = self:_point_to_key(channel.from_location)

         if new_parent_channels[key] == nil then
            new_parent_channels[key] = channel
         else
            log:warning('New parent already has a channel at %s', key)
         end

         local old_parent_channels = self:get_channels(old_parent)
         old_parent_channels[key] = nil
      end
   end
end

function ChannelManager:fill_channels_to_capacity()
   -- process channels in order of increasing elevation
   self:each_channel_ascending(function(channel)
         local water_component = channel.from_entity:add_component('stonehearth:water')
         water_component:_fill_channels_to_capacity()
      end)
end

function ChannelManager:empty_channels()
   local water_queue = {}

   self:each_channel(function(channel)
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

function ChannelManager:_create_water_queue_entry(entity, location, volume)
   local entry = {
      entity = entity,
      location = location,
      volume = volume
   }
   return entry
end

function ChannelManager:update_channel_types()
   self:each_channel(function(channel)
         assert(channel.queued_volume == 0)
         local water_component = channel.to_entity:add_component('stonehearth:water')
         local water_level = water_component:get_water_level()
         local channel_height = channel.from_location.y

         if channel.channel_type == 'pressure' then
            if water_level < channel_height then
               self:link_waterfall_channel(channel.from_entity, channel.from_location)
            end
         elseif channel.channel_type == 'waterfall' then
            if water_level > channel_height then
               local target_adjacent_point = self:_get_best_channel_adjacent_point(channel.from_location, channel.from_entity)
               self:link_pressure_channel(channel.from_entity, channel.from_location, channel.to_entity, target_adjacent_point)
            end
         else
            assert(false)
         end
      end)
end

function ChannelManager:update_channel_entities()
   self:each_channel(function(channel)
         if channel.channel_entity then
            local waterfall_component = channel.channel_entity:get_component('stonehearth:waterfall')
            if waterfall_component then
               waterfall_component:set_volume(channel.queued_volume)
            end
         end
      end)
end

function ChannelManager:sort_channels_ascending(channels)
   local meta_channels = {}
   for _, channel in pairs(channels) do
      -- extract the elevation so we don't keep going to c++ during the sort
      table.insert(meta_channels, { channel = channel, elevation = channel.from_location.y })
   end

   table.sort(meta_channels, function(a, b)
         return a.elevation < b.elevation
      end)

   local sorted_channels = {}
   for _, entry in ipairs(meta_channels) do
      table.insert(sorted_channels, entry.channel)
   end

   return sorted_channels
end

function ChannelManager:_point_to_key(point)
   return string.format('%d,%d,%d', point.x, point.y, point.z)
end

return ChannelManager
