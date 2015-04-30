local constants = require 'constants'
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local log = radiant.log.create_logger('water')

local ChannelManager = class()

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
   local channels = self:get_channels(from_entity)
   self:remove_channels(channels)

   self._sv._channels[from_entity:get_id()] = nil
   self.__saved_variables:mark_changed()
end

function ChannelManager:remove_channels_to_entity(to_entity)
   self:each_channel(function(channel)
         if channel.to_entity == to_entity then
            self:remove_channel(channel)
         end
      end)
end

function ChannelManager:get_channel(from_entity, channel_entrance)
   local channels = self:get_channels(from_entity)
   local key = channel_entrance:key_value()
   local channel = channels[key]   
   return channel
end

function ChannelManager:get_channels(from_entity)
   local channels = self._sv._channels[from_entity:get_id()]
   return channels
end

function ChannelManager:assert_no_channels_at(point)
   local key = point:key_value()
   for id, channels in pairs(self._sv._channels) do
      assert(not channels[key])
   end
end

-- from_entity, to_entity
-- source_location - point in the source water region that feeds the channel
-- channel_entrance - location outside the source water region where the water enters the channel
-- channel_exit - location in the target water region where the water is added
-- conceptually the water flows from the source_location to the channel_entrance, then through the channel to the channel_exit
function ChannelManager:add_channel(from_entity, to_entity, source_location, channel_entrance, channel_exit, channel_type, subtype, channel_entity)
   log:debug('Adding %s channel from %s to %s with source_location %s, channel_entrance %s, channel_exit %s',
             channel_type, from_entity, to_entity, source_location, channel_entrance, channel_exit)

   if stonehearth.hydrology.enable_paranoid_assertions then
      local from_entity_location = radiant.entities.get_world_grid_location(from_entity)
      local from_water_component = from_entity:add_component('stonehearth:water')
      local from_water_region = from_water_component:get_region():get():translated(from_entity_location)
      -- also check if source_location equals from_entity_location in case the water region is empty
      if not from_water_region:contains(source_location) and source_location ~= from_entity_location then
         -- this condition is sometimes ok, but flag it anyway so we understand how we got here
         local foo = 1
      end
      assert(from_water_region:contains(source_location) or source_location == from_entity_location)
      assert(not from_water_region:contains(channel_entrance))
      assert(not from_water_region:contains(channel_exit))
   end

   local channel = {
      from_entity = from_entity,
      to_entity = to_entity,
      source_location = source_location,
      channel_entrance = channel_entrance,
      channel_exit = channel_exit,
      channel_type = channel_type,
      subtype = subtype,
      channel_entity = channel_entity,
      queued_volume = 0,
      destroyed = false
   }

   local key = channel_entrance:key_value()
   local channels = self:get_channels(from_entity)
   channels[key] = channel

   self.__saved_variables:mark_changed()
   return channel
end

function ChannelManager:remove_channel(channel)
   local paired_channel = channel.paired_channel
   self:_remove_unidirectional_channel(channel)

   if paired_channel then
      self:_remove_unidirectional_channel(paired_channel)
   end
end

function ChannelManager:remove_channels(channels)
   for _, channel in pairs(channels) do
      self:remove_channel(channel)
   end
end

function ChannelManager:_remove_unidirectional_channel(channel)
   local channels = self:get_channels(channel.from_entity)
   local key = channel.channel_entrance:key_value()

   log:debug('Removing %s channel from %s at %s', channel.channel_type, channel.from_entity, channel.channel_entrance)

   channels[key] = nil
   self:_destroy_channel(channel)

   self.__saved_variables:mark_changed()
end

function ChannelManager:_destroy_channel(channel)
   if channel.channel_entity then
      radiant.entities.destroy_entity(channel.channel_entity)
   end
   channel.destroyed = true
   self.__saved_variables:mark_changed()
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
      log:spam('Added %g to channel for %s at %s', flow_volume, channel.from_entity, channel.channel_entrance)
   end

   self.__saved_variables:mark_changed()

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
      target_elevation = channel.channel_entrance.y
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
   local flow_rate = pressure * constants.hydrology.PRESSURE_TO_FLOW_RATE

   -- establish a minimum flow rate to avoid computing immaterial deltas
   -- also avoids exponential convergence
   if flow_rate < constants.hydrology.MIN_FLOW_RATE then
      if flow_rate < constants.hydrology.STOP_FLOW_THRESHOLD then
         flow_rate = 0
      else
         flow_rate = constants.hydrology.MIN_FLOW_RATE
      end
   end

   return flow_rate
end

function ChannelManager:link_waterfall_channel(from_entity, source_location, channel_entrance)
   log:debug('Linking waterfall channel from %s with source_location %s, channel_entrance %s',
             from_entity, source_location, channel_entrance)

   -- vertical waterfalls have their channel_entrance at the source_location
   local subtype = source_location.y ~= channel_entrance.y and 'vertical' or nil
   local channel = self:get_channel(from_entity, channel_entrance)

   if channel then
      -- if channel.channel_type == 'waterfall' then
      --    assert(channel.subtype == subtype)
      --    assert(channel.from_entity == from_entity)
      --    assert(channel.channel_entrance == channel_entrance)

      --    -- Multiple source locations are valid for the channel_entrance
      --    if (channel.source_location ~= source_location) then
      --       -- Remove the channel if the source location is gone
      --       if not self:_water_body_contains(from_entity, source_location) then
      --          self:remove_channel(channel)
      --          channel = nil
      --       end
      --    end
      -- else
         -- This removes both directions of the pressure channel
         self:remove_channel(channel)
         channel = nil
      -- end
   end

   if not channel then
      local to_entity, channel_exit = self:_get_waterfall_target(channel_entrance)

      if to_entity == from_entity then
         log:info('from_entity == to_entity for waterfall channel. ignoring.')
      else
         -- TODO: the channel_exit isn't particularly useful for a waterfall as it may change
         -- depending on what entity is the target when the watertight region for the waterfall column changes
         local waterfall = self:_create_waterfall(from_entity, to_entity, source_location, channel_entrance, channel_exit)
         channel = self:add_channel(from_entity, to_entity, source_location, channel_entrance, channel_exit, 'waterfall', subtype, waterfall)
      end
   end
   return channel
end

function ChannelManager:link_pressure_channel(from_entity, to_entity, from_location, to_location)
   log:debug('Linking pressure channel from %s to %s with from_location %s, to_location %s',
             from_entity, to_entity, from_location, to_location)

   local subtype = from_location.y ~= to_location.y and 'vertical' or nil

   local forward_channel = self:_link_pressure_channel_unidirectional(from_entity, to_entity, from_location, to_location, subtype)

   local reverse_channel = self:_link_pressure_channel_unidirectional(to_entity, from_entity, to_location, from_location, subtype)

   forward_channel.paired_channel = reverse_channel
   reverse_channel.paired_channel = forward_channel

   return forward_channel
end

function ChannelManager:_link_pressure_channel_unidirectional(from_entity, to_entity, from_location, to_location, subtype)
   -- By definition, the channel_entrance is the adjacent point outside of the water region
   local channel_entrance = to_location
   local channel = self:get_channel(from_entity, channel_entrance)

   -- If channel of the same type already exists, make sure it is identical, otherwise replace it
   if channel then
      -- if channel.channel_type == 'pressure' then
      --    assert(channel.from_entity == from_entity)
      --    assert(channel.to_entity == to_entity)
      --    assert(channel.channel_entrance == to_location)
      --    assert(channel.channel_exit == to_location)

      --    -- Multiple source locations are valid for the channel_entrance
      --    if (channel.source_location ~= from_location) then
      --       -- If the subtypes don't match, prefer the horizontal channel
      --       if channel.subtype ~= subtype and channel.subtype == 'vertical' then
      --          self:remove_channel(channel)
      --          channel = nil
      --       end

      --       -- Remove the channel if the source location is gone
      --       if channel and not self:_water_body_contains(from_entity, from_location) then
      --          self:remove_channel(channel)
      --          channel = nil
      --       end
      --    end
      -- else
         self:remove_channel(channel)
         channel = nil
      -- end
   end

   if not channel then
      channel = self:add_channel(from_entity, to_entity, from_location, to_location, to_location, 'pressure', subtype)
   end
   return channel
end

function ChannelManager:convert_to_pressure_channel(channel)
   assert(channel.channel_type == 'waterfall')
   log:info('Converting waterfall to pressure channel')

   -- TODO: the channel_entrance might not be in the to_entity...
   local new_channel = self:link_pressure_channel(channel.from_entity, channel.to_entity, channel.source_location, channel.channel_entrance)
   assert(channel.destroyed)

   if stonehearth.hydrology.enable_paranoid_assertions then
      self:check_all_channels()
   end

   return new_channel
end

function ChannelManager:convert_to_waterfall_channel(channel)
   assert(channel.channel_type == 'pressure')
   log:info('Converting pressure to waterfall channel')

   local new_channel = self:link_waterfall_channel(channel.from_entity, channel.source_location, channel.channel_entrance)
   assert(channel.destroyed)
   assert(channel.paired_channel.destroyed)

   if stonehearth.hydrology.enable_paranoid_assertions then
      self:check_all_channels()
   end

   return new_channel
end

function ChannelManager:find_new_channel_source(channel)
   local from_entity_location = radiant.entities.get_world_grid_location(channel.from_entity)
   local from_water_component = channel.from_entity:add_component('stonehearth:water')
   local old_source_location = channel.source_location

   log:debug('looking for new source_location for channel with old source_location of %s', old_source_location)

   assert(not from_water_component:get_region():get():contains(old_source_location - from_entity_location))

   local new_source_location = stonehearth.hydrology:get_best_source_location(channel.from_entity, channel.channel_entrance)
   local new_channel = nil

   if new_source_location then
      if channel.channel_type == 'waterfall' then
         new_channel = self:link_waterfall_channel(channel.from_entity, new_source_location, channel.channel_entrance)
      elseif channel.channel_type == 'pressure' then
         new_channel = self:link_pressure_channel(channel.from_entity, channel.to_entity, new_source_location, channel.channel_entrance)
      else
         assert(false)
      end
   else
      self:remove_channel(channel)
   end

   assert(channel.destroyed)
   if channel.paired_channel then
      assert(channel.paired_channel.destroyed)
   end

   return new_channel
end

function ChannelManager:_get_waterfall_target(channel_entrance)
   local region = Region3()
   region:add_point(channel_entrance)
   local projected_region = _physics:project_region(region, _radiant.physics.Physics.CLIP_SOLID)
   assert(not projected_region:empty())
   local channel_exit = projected_region:get_rect(0).min

   local to_entity = stonehearth.hydrology:get_water_body(channel_exit)
   if not to_entity then
      to_entity = stonehearth.hydrology:create_water_body(channel_exit)
   end
   return to_entity, channel_exit
end

function ChannelManager:_create_waterfall(from_entity, to_entity, source_location, channel_entrance, channel_exit)
   local waterfall = radiant.entities.create_entity('stonehearth:terrain:waterfall')
   local waterfall_component = waterfall:add_component('stonehearth:waterfall')

   radiant.terrain.place_entity_at_exact_location(waterfall, channel_entrance)
   waterfall_component:set_top_elevation(source_location.y)   -- source_location, not the channel_entrance
   waterfall_component:set_source(from_entity)
   waterfall_component:set_target(to_entity)
   return waterfall
end

function ChannelManager:merge_channels(master, mergee)
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

   if stonehearth.hydrology.enable_paranoid_assertions then
      self:check_all_channels()
   end
end

function ChannelManager:reparent_channels(channels, new_parent)
   for _, channel in pairs(channels) do
      self:reparent_channel(channel, new_parent)
   end
end

function ChannelManager:reparent_channel(channel, new_parent)
   if channel.to_entity == new_parent then
      -- channel would go to itself, so just remove it
      self:remove_channel(channel)
      return
   end

   local key = channel.channel_entrance:key_value()
   local old_parent = channel.from_entity
   local old_parent_channels = self:get_channels(old_parent)
   local new_parent_channels = self:get_channels(new_parent)

   if new_parent_channels[key] then
      log:warning('New parent already has a channel at %s', key)
      self:remove_channel(channel)
      return
   end

   old_parent_channels[key] = nil
   new_parent_channels[key] = channel

   -- assign from_entity and to_entity last so that remove_channel is clean
   channel.from_entity = new_parent

   if channel.paired_channel then
      channel.paired_channel.to_entity = new_parent
   end

   self.__saved_variables:mark_changed()
end

function ChannelManager:update_channels_on_source_location_removed(point, from_entity)
   -- TODO: waterfall targets should be checked, but this is probably ok for now
   local channels = self:get_channels(from_entity)

   for _, channel in pairs(channels) do
      if channel.source_location == point then
         self:find_new_channel_source(channel)
      end
   end
end

function ChannelManager:check_all_channels()
   self:each_channel(function(channel)
         self:check_channel_integrity(channel)
      end)
end

function ChannelManager:check_channel_integrity(channel)
   assert(not channel.destroyed)
   local from_entity_location = radiant.entities.get_world_grid_location(channel.from_entity)
   local from_entity_region = channel.from_entity:add_component('stonehearth:water'):get_region():get():translated(from_entity_location)
   assert(from_entity_region:contains(channel.source_location) or channel.source_location == from_entity_location)
   assert(not from_entity_region:contains(channel.channel_entrance))
   assert(not from_entity_region:contains(channel.channel_exit))

   if channel.channel_type == 'pressure' then
      local to_entity_location = radiant.entities.get_world_grid_location(channel.to_entity)
      local to_entity_region = channel.to_entity:add_component('stonehearth:water'):get_region():get():translated(to_entity_location)
      assert(to_entity_region:contains(channel.channel_entrance) or channel.channel_entrance == to_entity_location)
      assert(to_entity_region:contains(channel.channel_exit) or channel.channel_exit == to_entity_location)
   end

   local paired_channel = channel.paired_channel
   if paired_channel then
      assert(channel.from_entity == paired_channel.to_entity)
      assert(channel.to_entity == paired_channel.from_entity)
      assert(channel.channel_entrance == paired_channel.source_location)
      assert(channel.source_location == paired_channel.channel_entrance)
   end
end

function ChannelManager:fill_channels_to_capacity()
   -- process channels in order of increasing elevation
   self:each_channel_ascending(function(channel)
         local water_component = channel.from_entity:add_component('stonehearth:water')
         water_component:fill_channel_from_water_region(channel)
      end)

   self.__saved_variables:mark_changed()
end

function ChannelManager:empty_channels()
   local water_queue = {}

   self:each_channel(function(channel)
         if channel.queued_volume > 0 then
            local entry = self:_create_water_queue_entry(channel)
            table.insert(water_queue, entry)
         end

         -- empty the channel so we can queue for the next iteration
         channel.queued_volume = 0
      end)

   self.__saved_variables:mark_changed()

   return water_queue
end

function ChannelManager:_create_water_queue_entry(channel)
   local entry = {
      from_entity = channel.from_entity,
      to_entity = channel.to_entity,
      from_location = channel.source_location,
      to_location = channel.channel_exit,
      volume = channel.queued_volume
   }
   return entry
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
      table.insert(meta_channels, { channel = channel, elevation = channel.channel_entrance.y })
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

function ChannelManager:_water_body_contains(entity, point)
   local location = radiant.entities.get_world_grid_location(entity)
   local water_component = entity:add_component('stonehearth:water')
   local result = water_component:get_region():get():contains(point - location)
   return result
end

return ChannelManager
