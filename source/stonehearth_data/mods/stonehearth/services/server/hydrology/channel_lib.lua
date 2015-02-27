local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local log = radiant.log.create_logger('water')

local channel_lib = {}

-- constant converting pressure to a flow rate per unit cross section
local PRESSURE_TO_FLOW_RATE = 1

-- a 'drop' of water
local MIN_FLOW_RATE = 0.01

function channel_lib.add_volume_to_channel(channel, volume, source_elevation_bias)
   assert(volume >= 0)

   -- get the flow volume per tick
   local max_flow_volume = channel_lib.calculate_channel_flow_rate(channel, source_elevation_bias)

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
function channel_lib.calculate_channel_flow_rate(channel, source_elevation_bias)
   source_elevation_bias = source_elevation_bias or 0

   local from_water_component = channel.from_entity:add_component('stonehearth:water')
   local source_elevation = from_water_component:get_water_elevation() + source_elevation_bias
   local target_elevation

   if channel.channel_type == 'waterfall' then
      target_elevation = channel.from_location.y
   elseif channel.channel_type == 'pressure' then
      local to_water_component = channel.to_entity:add_component('stonehearth:water')
      target_elevation = to_water_component:get_water_elevation()
   else
      assert(false)
   end

   local flow_rate = channel_lib.calculate_flow_rate(source_elevation, target_elevation)
   return flow_rate
end

-- returns flow rate in voxels / tick / unit square
-- flow rate may be negative
function channel_lib.calculate_flow_rate(from_elevation, to_elevation)
   local pressure = from_elevation - to_elevation
   local flow_rate = pressure * PRESSURE_TO_FLOW_RATE

   -- stop flowing when less than a "drop" of water
   -- we don't want to keep computing immaterial deltas
   if flow_rate < MIN_FLOW_RATE then
      flow_rate = 0
   end

   return flow_rate
end

function channel_lib.sort_channels_ascending(channels)
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

function channel_lib.link_waterfall_channel(from_entity, from_location)
   local channel = stonehearth.hydrology:get_channel(from_entity, from_location)

   if channel and channel.channel_type ~= 'waterfall' then
      assert(channel.channel_type == 'pressure')
      -- remove both directions of the pressure channel
      stonehearth.hydrology:remove_channel(from_entity, from_location)
      stonehearth.hydrology:remove_channel(to_entity, to_location)
      channel = nil
   end

   if not channel then
      local to_entity, to_location = channel_lib._get_waterfall_target(from_location)
      local waterfall = channel_lib._create_waterfall(from_entity, from_location, to_entity, to_location)
      channel = stonehearth.hydrology:add_channel(from_entity, from_location, to_entity, to_location, 'waterfall', waterfall)
   end
   return channel
end

-- Confusing, but the source_adjacent_point is inside the target and the target_adjacent_point
-- is inside the source. This is because the from_location of the channel is defined to be outside
-- the region of the source water body.
function channel_lib.link_pressure_channel(source_entity, source_adjacent_point, target_entity, target_adjacent_point)
   local forward_channel = channel_lib._link_pressure_channel_unidirectional(source_entity, source_adjacent_point,
                                                                             target_entity, source_adjacent_point)

   local reverse_channel = channel_lib._link_pressure_channel_unidirectional(target_entity, target_adjacent_point,
                                                                             source_entity, target_adjacent_point)

   return forward_channel
end

function channel_lib._link_pressure_channel_unidirectional(from_entity, from_location, to_entity, to_location)
   -- note that the to_location is the same as the from_location
   local channel = stonehearth.hydrology:get_channel(from_entity, from_location)

   if channel and channel.channel_type ~= 'pressure' then
      stonehearth.hydrology:remove_channel(from_entity, from_location)
      channel = nil
   end

   if not channel then
      channel = stonehearth.hydrology:add_channel(from_entity, from_location, to_entity, to_location, 'pressure', nil)
   end
   return channel
end

function channel_lib._get_waterfall_target(from_location)
   local to_location = radiant.terrain.get_point_on_terrain(from_location)
   local to_entity = stonehearth.hydrology:get_water_body(to_location)
   if not to_entity then
      to_entity = stonehearth.hydrology:create_water_body(to_location)
   end
   return to_entity, to_location
end

function channel_lib._create_waterfall(from_entity, from_location, to_entity, to_location)
   local waterfall = radiant.entities.create_entity('stonehearth:terrain:waterfall')
   radiant.terrain.place_entity_at_exact_location(waterfall, from_location)
   local waterfall_component = waterfall:add_component('stonehearth:waterfall')
   waterfall_component:set_height(from_location.y - to_location.y)
   waterfall_component:set_source(from_entity)
   waterfall_component:set_target(to_entity)
   return waterfall
end

return channel_lib
