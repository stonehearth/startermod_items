local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local log = radiant.log.create_logger('water')

HydrologyService = class()

function HydrologyService:initialize()
   self._sv = self.__saved_variables:get_data()

   if not self._sv._initialized then
      self._sv._water_bodies = {}
      self._sv._water_queue = {}
      self._sv._flows = {}
      self._sv._initialized = true
      self.__saved_variables:mark_changed()
   else
   end

   stonehearth.calendar:set_interval(20, function()
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
   self._sv._water_queue[id] = {}
   self._sv._flows[id] = {}
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
   self._sv._water_queue[id] = nil
   self._sv._flows[id] = nil
   self.__saved_variables:mark_changed()
end

function HydrologyService:create_waterfall(from_location, to_location)
   local entity = radiant.entities.create_entity('stonehearth:terrain:waterfall')
   radiant.terrain.place_entity_at_exact_location(entity, from_location)
   local waterfall_component = entity:add_component('stonehearth:waterfall')
   waterfall_component:set_height(from_location.y - to_location.y)
   return entity
end

function HydrologyService:get_flows(from_entity)
   local flows = self._sv._flows[from_entity:get_id()]
   return flows
end

function HydrologyService:add_flow(from_entity, from_location, to_entity, to_location, channel_entity)
   log:debug('adding flow from %s at %s to %s at %s', from_entity, from_location, to_entity, to_location)

   local from_entity_id = from_entity:get_id()
   local flow = {
      from_entity = from_entity,
      from_location = from_location,
      to_entity = to_entity,
      to_location = to_location,
      channel_entity = channel_entity
   }

   local flows = self._sv._flows[from_entity_id]
   local key = self:_point_to_key(from_location)
   flows[key] = flow

   self.__saved_variables:mark_changed()
   return flow
end

function HydrologyService:remove_flow(from_entity, from_location)
   log:debug('removing flow from %s at %s', from_entity, from_location)

   local from_entity_id = from_entity:get_id()
   local flows = self._sv._flows[from_entity_id]

   local key = self:_point_to_key(from_location)
   local flow = flows[key]

   if flow then
      self:_destroy_flow(flow)
      flows[key] = nil
   end

   self.__saved_variables:mark_changed()
end

function HydrologyService:_destroy_flow(flow)
   radiant.entities.destroy_entity(flow.channel_entity)
end

function HydrologyService:queue_water(entity, location, volume)
   local id = entity:get_id()
   local queue = self._sv._water_queue[id]

   log:spam('queuing %s at %s with volume %d', entity, location, volume)

   local entry = {
      location = location,
      volume = volume
   }
   table.insert(queue, entry)
end

function HydrologyService:merge_water_bodies(master, mergee)
   local master_location = radiant.entities.get_world_grid_location(master)
   local mergee_location = radiant.entities.get_world_grid_location(mergee)
   log:debug('merging %s at %s with %s at %s', master, master_location, mergee, mergee_location)

   self:_merge_regions(master, mergee)
   self:_merge_water_queues(master, mergee)
   self:_merge_flows(master, mergee)

   self:remove_water_body(mergee)
   radiant.entities.destroy_entity(mergee)
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

function HydrologyService:_merge_water_queues(master, mergee)
   local master_id = master:get_id()
   local mergee_id = mergee:get_id()
   local master_queue = self._sv._water_queue[master_id]
   local mergee_queue = self._sv._water_queue[mergee_id]

   for _, entry in pairs(mergee_queue) do
      table.insert(master_queue, entry)
   end

   self._sv._water_queue[mergee_id] = {}
end

function HydrologyService:_merge_flows(master, mergee)
   local master_id = master:get_id()
   local mergee_id = mergee:get_id()
   local master_flows = self._sv._flows[master_id]
   local mergee_flows = self._sv._flows[mergee_id]

   -- all flows originating from mergee now originate from master
   for key, flow in pairs(mergee_flows) do
      if flow.to_entity == master then
         -- destroy the flow if it would redirect to itself
         self:_destroy_flow(flow)
      else
         -- the flow now originates from the master entity
         flow.from_entity = master
      end
      assert(not master_flows[key])
      master_flows[key] = flow
   end

   -- remove the flows that had originated from mergee
   self._sv._flows[mergee_id] = {}

   -- all flows going to mergee now go to master
   for id, flows in pairs(self._sv._flows) do
      if id ~= master_id then
         for key, flow in pairs(flows) do
            if flow.to_entity == mergee then
               flow.to_entity = master
            end
         end
      end
   end
end

function HydrologyService:_on_tick()
   log:spam('processing next tick')

   local current_queue = {}
   for id, entries in pairs(self._sv._water_queue) do
      -- save the current queue for processing below
      current_queue[id] = entries
      -- clear the queue so we can use it to buffer for the next tick
      self._sv._water_queue[id] = {}
   end
   
   for id, entries in pairs(current_queue) do
      local entity = self._sv._water_bodies[id]
      local water_component = entity:add_component('stonehearth:water')

      for _, entry in pairs(entries) do
         water_component:add_water(entry.location, entry.volume)
      end
   end
end

function HydrologyService:_point_to_key(point)
   return string.format('%d,%d,%d', point.x, point.y, point.z)
end

return HydrologyService
