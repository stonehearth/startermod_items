local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local log = radiant.log.create_logger('water')

HydrologyService = class()

function HydrologyService:initialize()
   self._sv = self.__saved_variables:get_data()

   if not self._sv._initialized then
      self._sv._water_bodies = {}
      self._sv._queue_map = {}
      self._sv._channels = {}
      self._sv._next_id = 1
      self._sv._initialized = true
      self.__saved_variables:mark_changed()
   else
   end

   stonehearth.calendar:set_interval(10, function()
      self:_on_tick()
   end)
end

function HydrologyService:destroy()
end

function HydrologyService:create_water_body(location)
   local entity = radiant.entities.create_entity('stonehearth:terrain:water')
   log:debug('creating water body %s at %s', entity, location)

   local id = entity:get_id()
   self._sv._water_bodies[id] = entity
   self._sv._queue_map[id] = {}
   radiant.terrain.place_entity_at_exact_location(entity, location)
   self.__saved_variables:mark_changed()

   return entity
end

function HydrologyService:get_water_body(location)
   for id, entity in pairs(self._sv._water_bodies) do
      local entity_location = radiant.entities.get_world_grid_location(entity)
      local local_location = location - entity_location
      local water_component = entity:add_component('stonehearth:water')
      local region = water_component:get_region()

      -- cache the region bounds as a quick rejection test if this becomes slow
      if region:contains(local_location) then
         return entity
      end
   end

   return nil
end

function HydrologyService:remove_water_body(entity)
   log:debug('removing water body %s', entity)

   local id = entity:get_id()
   self._sv._water_bodies[id] = nil
   self._sv._queue_map[id] = nil
   self.__saved_variables:mark_changed()
end

function HydrologyService:create_waterfall_channel(from_entity, from_location)
   local to_location = radiant.terrain.get_point_on_terrain(from_location)
   local to_entity = self:get_water_body(to_location)
   if not to_entity then
      to_entity = self:create_water_body(to_location)
   end

   local waterfall = self:_create_waterfall(from_location, to_location)
   local channel = self:add_channel(from_entity, from_location, to_entity, to_location, waterfall)
   return channel
end

function HydrologyService:_create_waterfall(from_location, to_location)
   local entity = radiant.entities.create_entity('stonehearth:terrain:waterfall')
   radiant.terrain.place_entity_at_exact_location(entity, from_location)
   local waterfall_component = entity:add_component('stonehearth:waterfall')
   waterfall_component:set_height(from_location.y - to_location.y)
   return entity
end

function HydrologyService:get_channel(from_entity, from_location)
   -- TODO: optimize to avoid O(n) iteration
   for _, channel in pairs(self._sv._channels) do
      if channel.from_location == from_location and channel.from_entity == from_entity then
         return channel
      end
   end
   return nil
end

-- to_entity is an optional hint
function HydrologyService:add_water(volume, to_location, to_entity)
   if not to_entity then
      to_entity = self:get_water_body(to_location)
   end

   log:debug('adding channel to %s at %s', to_entity, to_location)

   local channel = self:_create_channel(nil, nil, to_entity, to_location, nil, false)
   channel.queued_volume = volume
   self._sv._channels[channel.id] = channel
end

function HydrologyService:add_channel(from_entity, from_location, to_entity, to_location, channel_entity)
   log:debug('adding channel from %s at %s to %s at %s', from_entity, from_location, to_entity, to_location)

   local channel = self:_create_channel(from_entity, from_location, to_entity, to_location, channel_entity, true)
   self._sv._channels[channel.id] = channel

   self.__saved_variables:mark_changed()
   return channel
end

function HydrologyService:remove_channel(id)
   local channel = self._sv._channels[id]
   if not channel then
      return
   end

   self._sv._channels[id] = nil
   self:_destroy_channel(channel)

   if channel.from_entity then
      log:debug('removing channel (%d) from %s at %s', id, from_entity, from_location)
   else
      log:debug('removing channel (%d)', id)
   end

   self.__saved_variables:mark_changed()
end

function HydrologyService:_destroy_channel(channel)
   if channel.channel_entity then
      radiant.entities.destroy_entity(channel.channel_entity)
   end
end

function HydrologyService:merge_water_bodies(master, mergee)
   local master_location = radiant.entities.get_world_grid_location(master)
   local mergee_location = radiant.entities.get_world_grid_location(mergee)
   log:debug('merging %s at %s with %s at %s', master, master_location, mergee, mergee_location)

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
   for id, channel in pairs(self._sv._channels) do
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
   end
end

function HydrologyService:_merge_water_queue(master, mergee)
   if not self._water_queue then
      return
   end

   for _, entry in pairs(self._water_queue) do
      if entry.to_entity == mergee then
         entry.to_entity = master
      end
   end
end

function HydrologyService:_on_tick()
   log:spam('processing next tick')

   self._water_queue = self:_empty_channels()

   for _, entry in pairs(self._water_queue) do
      log:spam('on_tick adding %d to %s at %s', entry.volume, entry.entity, entry.location)

      local water_component = entry.entity:add_component('stonehearth:water')
      -- this call may merge water bodies and cause the entries in the water queue to change
      water_component:add_water(entry.location, entry.volume)
   end

   self._water_queue = nil

   self:_update_channels()

   self.__saved_variables:mark_changed()
end

function HydrologyService:_empty_channels()
   local water_queue = {}

   for _, channel in pairs(self._sv._channels) do
      local entry = {
         entity = channel.to_entity,
         location = channel.to_location,
         volume = channel.queued_volume
      }
      table.insert(water_queue, entry)

      if not channel.persistent then
         self._sv._channels[channel.id] = nil
      else
         -- empty the channel so we can queue for the next iteration
         channel.queued_volume = 0
      end
   end

   self.__saved_variables:mark_changed()

   return water_queue
end

function HydrologyService:_update_channels()
   for _, channel in pairs(self._sv._channels) do
      local waterfall_component = channel.channel_entity:add_component('stonehearth:waterfall')
      if waterfall_component then
         waterfall_component:set_volume(channel.queued_volume)
      end
   end
end

function HydrologyService:_create_channel(from_entity, from_location, to_entity, to_location, channel_entity, persistent)
   local channel = {
      id = self:_get_next_id(),
      from_entity = from_entity,
      from_location = from_location,
      to_entity = to_entity,
      to_location = to_location,
      channel_entity = channel_entity,
      persistent = persistent,
      queued_volume = 0
   }

   self.__saved_variables:mark_changed()

   return channel
end

function HydrologyService:_point_to_key(point)
   return string.format('%d,%d,%d', point.x, point.y, point.z)
end

function HydrologyService:_get_next_id()
   local id = self._sv._next_id
   self._sv._next_id = id + 1
   self.__saved_variables:mark_changed()
   return id
end

return HydrologyService
