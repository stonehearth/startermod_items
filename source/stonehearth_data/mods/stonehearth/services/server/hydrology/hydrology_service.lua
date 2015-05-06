local constants = require 'constants'
local csg_lib = require 'lib.csg.csg_lib'
local ChannelManager = require 'services.server.hydrology.channel_manager'
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local log = radiant.log.create_logger('water')
local TraceCategories = _radiant.dm.TraceCategories

HydrologyService = class()

function HydrologyService:initialize()
   -- prevent oscillation by making sure we can flow to under the merge threshold
   assert(constants.hydrology.MIN_FLOW_RATE <= constants.hydrology.MERGE_ELEVATION_THRESHOLD)
   assert(constants.hydrology.MIN_FLOW_RATE <= constants.hydrology.MERGE_VOLUME_THRESHOLD)
   assert(constants.hydrology.MIN_FLOW_RATE >= constants.hydrology.STOP_FLOW_THRESHOLD)

   self.enable_paranoid_assertions = radiant.util.get_config('water.enable_paranoid_assertions', false)

   self._sv = self.__saved_variables:get_data()

   if not self._sv._initialized then
      self._sv._water_bodies = {}
      self._sv._channel_manager = radiant.create_controller('stonehearth:channel_manager')
      self._sv._initialized = true
      self.__saved_variables:mark_changed()
   else
      radiant.events.listen_once(radiant, 'radiant:game_loaded', function()
            self:_deferred_initialize()
         end)
   end
end

function HydrologyService:start()
   self:_deferred_initialize()
end

function HydrologyService:_deferred_initialize()
   self:_trace_terrain_delta()

   if radiant.util.get_config('enable_water', true) then
      if self._sv.water_tick then
         self._sv.water_tick:bind(function()
               self:_on_tick()
            end)
      else
         self._sv.water_tick = stonehearth.calendar:set_interval(10, function()
               self:_on_tick()
            end)
         end
   end
end

function HydrologyService:destroy()
   radiant.destroy_controller(self._sv._channel_manager)

   self:_destroy_terrain_delta_trace()
end

function HydrologyService:get_channel_manager()
   return self._sv._channel_manager
end

function HydrologyService:get_water_bodies()
   return self._sv._water_bodies
end

function HydrologyService:get_water_tight_region()
   return self._water_tight_region
end

function HydrologyService:_trace_terrain_delta()
   local terrain_component = radiant.terrain.get_terrain_component()
   self._water_tight_region = terrain_component:get_water_tight_region()

   -- The water_tight_region_delta will contain the entire terrain and all watertight shapes from world generation/loading.
   -- Clear the delta since we're only interested in changes in the water tight region from gameplay events.
   terrain_component:clear_water_tight_region_delta()

   self._delta_trace = terrain_component:trace_water_tight_region_delta('hydrology service', TraceCategories.SYNC_TRACE)
      :on_changed(function(delta_region)
            self:_on_terrain_changed(delta_region)
         end)
end

function HydrologyService:_destroy_terrain_delta_trace()
   if self._delta_trace then
      self._delta_trace:destroy()
      self._delta_trace = nil
   end
end

function HydrologyService:_get_water_bodies_descending()
   local temp = {}

   for id, entity in pairs(self._sv._water_bodies) do
      local water_level = entity:add_component('stonehearth:water'):get_water_level()
      local entry = { entity = entity, water_level = water_level, id = entity:get_id() }
      table.insert(temp, entry)
   end

   table.sort(temp, function(a, b)
      if a.water_level ~= b.water_level then
         return a.water_level < b.water_level
      end
      return a.id < b.id
   end)

   local water_bodies = {}

   for _, entry in ipairs(temp) do
      table.insert(water_bodies, entry.entity)
   end

   return water_bodies
end

function HydrologyService:_on_terrain_changed(delta_region)
   if delta_region:empty() then
      return
   end

   log:info('water tight delta region %s', tostring(delta_region))
   
   local inflated_delta_bounds = self:_get_inflated_delta_bounds(delta_region)
   if not inflated_delta_bounds then
      return
   end

   local channel_manager = self:get_channel_manager()
   local blocks_to_link = {}
   local added_volumes = {}
   local water_bodies = self:_get_water_bodies_descending()

   for _, entity in ipairs(water_bodies) do
      -- fast-ish rejection test to see if the delta region modifies the water region or its container
      if self:_bounds_intersects_water_body(inflated_delta_bounds, entity) then
         local modified_water_region = self:_get_affected_water_region(delta_region, entity)

         for point in modified_water_region:each_point() do
            -- check that block was in fact added
            if self._water_tight_region:contains(point) then
               -- Someone placed a watertight block in the water. Just mimic the displacement for now.
               -- TODO: remove affected channels
               -- TODO: update top layer
               -- TODO: merge may have occured
               -- TODO: check for unused volume
               -- TODO: check for bisection at end
               -- TODO: if region is empty, delete water body
               self:add_water(1, point, entity)
            end
         end

         local location = radiant.entities.get_world_grid_location(entity)
         local water_component = entity:add_component('stonehearth:water')

         -- TODO: Include the top boundary for analysis. This can occur when a subsection section
         -- of the water body has a ceiling.
         local modified_container_region = self:_get_affected_container_region(delta_region, entity)

         for point in modified_container_region:each_point() do
            local point_in_wetting_layer = point.y == water_component:get_top_layer_elevation() and 
                                           water_component:top_layer_in_wetting_mode()

            if not point_in_wetting_layer then
               local point_key = point:key_value()
               -- all container points that have been added to water bodies will be in this map
               local processed = blocks_to_link[point_key] ~= nil

               -- check that block was in fact removed
               if not processed and not self._water_tight_region:contains(point) then
                  local source_location = self:get_best_source_location(entity, point)
                  assert(source_location)
                  local channel

                  if point.y == source_location.y then
                     -- add point to the water region
                     log:debug('%s removed from watertight region. Adding to %s', point, entity)

                     if stonehearth.hydrology.enable_paranoid_assertions then
                        assert(not self:get_water_body(point))
                        channel_manager:assert_no_channels_at(point)
                     end

                     water_component:add_to_region(Region3(Cube3(point - location)))
                     self:_increment_added_volumes(added_volumes, entity)
                     self:_add_block_to_link(blocks_to_link, point, entity)
                  elseif point.y == source_location.y - 1 then
                     -- bottom of the container
                     log:debug('%s removed from watertight region and adding point above to link list for %s', point, entity)
                     -- we didn't add the containter point to the water region, so add the water region point for linking
                     self:_add_block_to_link(blocks_to_link, point + Point3.unit_y, entity)
                  elseif point.y == source_location.y + 1 then
                     -- top of the container
                     assert(false, 'not implemented')
                  end
                  -- TODO: consider calling add_volume_to_channel later
                  
               end
            end
         end
         -- TODO: remove channels that are no longer adjacent to the entity
      end
   end

   -- For each block in list, check adjacent blocks for channels and transients
   self:_link_blocks(blocks_to_link)

   -- We added volume in new blocks to the region, so remove volume in water to keep the total volume
   -- the same and lower the water level. Do this after linking channels because blocks_to_link is
   -- holding references to entities that may split.
   self:_remove_added_volumes(added_volumes)
end

function HydrologyService:_add_block_to_link(blocks_to_link, block, entity)
   local block_key = block:key_value()
   local entry = { block = block, entity = entity }
   blocks_to_link[block_key] = entry
end

function HydrologyService:_link_blocks(blocks_to_link)
   for _, entry in pairs(blocks_to_link) do
      self:_link_channels_for_block(entry.block, entry.entity)
   end
end

function HydrologyService:_increment_added_volumes(added_volumes, entity)
   local id = entity:get_id()
   local volume = added_volumes[id] or 0
   volume = volume + 1
   added_volumes[id] = volume
end

function HydrologyService:_remove_added_volumes(added_volumes)
   for id, volume in pairs(added_volumes) do
      local entity = radiant.entities.get_entity(id)
      local water_component = entity:add_component('stonehearth:water')
      water_component:_remove_water(volume)
   end
end

-- can be faster
function HydrologyService:_link_channels_for_block(point, entity)
   local channel_manager = self:get_channel_manager()

   for _, direction in ipairs(csg_lib.XYZ_DIRECTIONS) do
      local vertical = direction.y ~= 0
      local adjacent_point = point + direction
      local adjacent_is_solid = self._water_tight_region:contains(adjacent_point)

      if not adjacent_is_solid then
         local adjacent_entity = self:get_water_body(adjacent_point)
         local channel

         if adjacent_entity then
            -- don't process the entity that we're part of
            if adjacent_entity ~= entity then
               channel = channel_manager:link_pressure_channel(entity, adjacent_entity, point, adjacent_point)
            end
         else
            -- TODO: support up direction
            if direction.y ~= 1 then
               local below_adjacent_point = adjacent_point - Point3.unit_y

               -- if supported or above, create entity and add pressure channel
               if self._water_tight_region:contains(below_adjacent_point) then
                  adjacent_entity = self:create_water_body(adjacent_point)
                  channel = channel_manager:link_pressure_channel(entity, adjacent_entity, point, adjacent_point)
               else
                  local below_adjacent_entity = self:get_water_body(below_adjacent_point)
                  local below_adjacent_water_level = nil
                  if below_adjacent_entity then
                     local below_adjacent_water_component = below_adjacent_entity:add_component('stonehearth:water')
                     below_adjacent_water_level = below_adjacent_water_component:get_water_level()
                  end

                  local transient = below_adjacent_entity and below_adjacent_water_level >= point.y

                  if transient then
                     -- TODO: add to transient list
                     assert(false, 'not implemented')
                  else
                     -- drop, create waterfall channel
                     assert(direction.y ~= 1, 'not implemented') -- TODO: not yet implemented
                     local channel_entrance = vertical and below_adjacent_point or adjacent_point
                     channel = channel_manager:link_waterfall_channel(entity, point, channel_entrance)
                  end
               end
            end
         end
      end
   end

end

-- can be faster if necessary
function HydrologyService:get_best_source_location(entity, point)
   local entity_location = radiant.entities.get_world_grid_location(entity)
   local water_component = entity:add_component('stonehearth:water')
   local water_region = water_component:get_region():get()

   local point_region = Region3()
   -- to local coordinates
   point_region:add_point(point - entity_location)
   local source_location = nil

   local inflated = csg_lib.get_non_diagonal_xz_inflated_region(point_region)
   local intersection = inflated:intersect_region(water_region)
   if not intersection:empty() then
      source_location = intersection:get_closest_point(point)
   end

   if not source_location then
      inflated = point_region:inflated(Point3.unit_y)
      intersection = inflated:intersect_region(water_region)
      if not intersection:empty() then
         source_location = intersection:get_closest_point(point)
      end
   end

   if not source_location then
      -- point is not adjacent to entity
      return nil
   end

   -- back to world coordinates
   local world_source_location = source_location + entity_location
   return world_source_location
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

-- TODO: Reconcile whether water body starts with a non-empty region.
-- Some merges want to merge with a water body that doesn't have water yet so that we can call
-- get_water_body. Terrain modifications want to create empty water bodies and let the channels
-- test to fill them.
function HydrologyService:create_water_body(location)
   return self:_create_water_body_internal(location)
end

-- Optimzied path to create a water body that is already filled.
-- Does not check that region is contained by a watertight boundary.
-- Know what you are doing before calling this.
function HydrologyService:create_water_body_with_region(region, height)
   assert(not region:empty())
   local boxed_region = _radiant.sim.alloc_region3()
   local location = self:select_origin_for_region(region)
   
   boxed_region:modify(function(cursor)
         cursor:copy_region(region)
         cursor:translate(-location)
      end)

   return self:_create_water_body_internal(location, boxed_region, height)
end

function HydrologyService:_create_water_body_internal(location, boxed_region, height)
   if boxed_region == nil then
      boxed_region = _radiant.sim.alloc_region3()
      height = 0
   end

   local entity = radiant.entities.create_entity('stonehearth:terrain:water')
   log:debug('Creating water body %s at %s', entity, location)

   local rcs_component = entity:add_component('region_collision_shape')
   rcs_component:set_region(boxed_region)
   rcs_component:set_region_collision_type( _radiant.om.RegionCollisionShape.NONE)

   local destination_component = entity:add_component('destination')
   destination_component:set_region(_radiant.sim.alloc_region3())
   destination_component:set_adjacent(_radiant.sim.alloc_region3())

   local water_component = entity:add_component('stonehearth:water')
   water_component:set_region(boxed_region, height)

   local id = entity:get_id()
   self._sv._water_bodies[id] = entity
   self._sv._channel_manager:allocate_channels(entity)

   radiant.terrain.place_entity_at_exact_location(entity, location)

   self.__saved_variables:mark_changed()

   return entity
end

function HydrologyService:select_origin_for_region(region)
   if region:empty() then
      return nil
   end

   local bounds = region:get_bounds()
   local bottom_slice = Cube3(bounds)
   bottom_slice.max.y = bottom_slice.min.y + 1

   -- origin needs to be at the bottom of the region, preferably towards the center
   local footprint = region:intersect_cube(bottom_slice)
   local centroid = _radiant.csg.get_region_centroid(footprint):to_closest_int()
   -- concave shapes may have centroids outside the region
   local origin = footprint:get_closest_point(centroid)
   return origin
end

-- O(n) on the number of cubes in all water bodies
-- can be much faster by caching bounds of water bodies
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

   -- TODO: are we sure we want to do this?
   -- if no water bodies contain the location, check for empty water bodies with origins at the location
   for id, entity in pairs(self._sv._water_bodies) do
      local entity_location = radiant.entities.get_world_grid_location(entity)
      if entity_location == location then
         return entity
      end
   end

   return nil
end

function HydrologyService:destroy_water_body(entity)
   log:debug('Removing water body %s', entity)

   local id = entity:get_id()
   self._sv._water_bodies[id] = nil
   self._sv._channel_manager:remove_channels_to_entity(entity)
   self._sv._channel_manager:deallocate_channels(entity)
   radiant.entities.destroy_entity(entity)

   self.__saved_variables:mark_changed()
end

-- entity is an optional hint
-- returns volume of water that could not be added
function HydrologyService:add_water(volume, location, entity)
   if volume <= 0 then
      return volume
   end

   if not entity then
      entity = self:get_water_body(location)
   end

   if not entity then
      entity = self:create_water_body(location)
   end

   local water_component = entity:add_component('stonehearth:water')
   local volume, info = water_component:_add_water(location, volume)

   if volume > 0 then
      if info.result == 'merge' then
         local master = self:merge_water_bodies(info.entity1, info.entity2)
         -- add the remaining water to the master
         return self:add_water(volume, location, master)
      else
         log:detail('could not add water because: %s', info.reason)
      end
   end

   return volume, info
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

   -- TODO: consider if these need to get cleaned up if they stay empty
   if water_component1:get_region():get():empty() or water_component2:get_region():get():empty() then
      return false
   end

   -- TODO: imrpove merging of 3.99 and 4.00 height water bodies
   -- only mergable if the top layers are at the same elevation
   local layer_elevation1 = water_component1:get_top_layer_elevation()
   local layer_elevation2 = water_component2:get_top_layer_elevation()
   if layer_elevation1 ~= layer_elevation2 then
      return false
   end

   -- must do this after elevation test because of floating point precision
   if elevation_delta == 0 then
      return true
   end

   -- if transferring a small volume of water through the channel would equalize heights, then allow merge
   -- TODO: consider optimizing repeated calls to O(n) get_area()
   if elevation_delta < constants.hydrology.MERGE_ELEVATION_THRESHOLD then
      local entity1_layer_area = water_component1._sv._top_layer:get():get_area()
      local entity2_layer_area = water_component2._sv._top_layer:get():get_area()
      local entity1_delta_height = constants.hydrology.MERGE_VOLUME_THRESHOLD / entity1_layer_area
      local entity2_delta_height = constants.hydrology.MERGE_VOLUME_THRESHOLD / entity2_layer_area
      if entity1_delta_height + entity2_delta_height >= elevation_delta then
         assert(elevation_delta < 1)
         return true
      end
   end

   return false
end

-- You better know what you're doing if allow_uneven_top_layers is true!
function HydrologyService:merge_water_bodies(entity1, entity2, allow_uneven_top_layers)
   if allow_uneven_top_layers == nil then
      allow_uneven_top_layers = false
   end
   assert(entity1 ~= entity2)
   local master, mergee = self:_order_entities(entity1, entity2)
   self:_merge_water_bodies_impl(master, mergee, allow_uneven_top_layers)
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

function HydrologyService:_merge_water_bodies_impl(master, mergee, allow_uneven_top_layers)
   local master_water_component = master:add_component('stonehearth:water')
   master_water_component:merge_with(mergee, allow_uneven_top_layers)

   self:_merge_water_queue(master, mergee)

   self:destroy_water_body(mergee)

   self.__saved_variables:mark_changed()
end

function HydrologyService:_merge_water_queue(master, mergee)
   if not self._water_queue then
      return
   end

   -- redirect all mergee references to master
   for _, entry in ipairs(self._water_queue) do
      if entry.from_entity == mergee then
         entry.from_entity = master
      end

      if entry.to_entity == mergee then
         entry.to_entity = master
      end

      -- Ok if from_entity now equals to_entity. We have queued water that needs to go somewhere!
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
   self:_update_channel_types()

   for i, entry in ipairs(self._water_queue) do
      local unused_volume = self:add_water(entry.volume, entry.to_location, entry.to_entity)
      if unused_volume > 0 then
         -- add the water back to where it came from
         self:add_water(unused_volume, entry.from_location, entry.from_entity)

         -- what else to we need to test for before merging?
         log:info('%s is fully bounded and is merging with %s', entry.to_entity, entry.from_entity)
         self:merge_water_bodies(entry.from_entity, entry.to_entity, true)
      end
   end

   self._water_queue = nil

   -- some water bodies are created, but water is never added to them, so get rid of them here
   -- also destroys water bodies that become empty through other processes
   self:_destroy_unused_water_bodies()

   log:spam('End tick')

   self.__saved_variables:mark_changed()
end

function HydrologyService:_destroy_unused_water_bodies()
   local target_water_bodies = {}

   -- create a map of water bodies that are targets of channels
   self._sv._channel_manager:each_channel(function(channel)
         target_water_bodies[channel.to_entity:get_id()] = true
      end)

   for id, entity in pairs(self._sv._water_bodies) do
      -- remove the water body if the region is empty and it is not a channel target
      if not target_water_bodies[id] then
         local water_component = entity:add_component('stonehearth:water')
         if water_component:get_region():get():empty() then
            self:destroy_water_body(entity)
         end
      end
   end
end

function HydrologyService:_check_for_channel_merge()
   -- TODO: can we clean this up?
   -- TODO: only check once for bidirectional channels
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

function HydrologyService:_update_channel_types()
   for id, entity in pairs(self._sv._water_bodies) do
      local water_component = entity:add_component('stonehearth:water')
      water_component:update_channel_types()
   end
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
