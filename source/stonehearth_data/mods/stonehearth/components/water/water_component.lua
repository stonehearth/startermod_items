local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local log = radiant.log.create_logger('water')

local WaterComponent = class()

function WaterComponent:__init()
   -- the volume of water consumed to make a block wet
   self._wetting_volume = 0.25
end

function WaterComponent:initialize(entity, json)
   self._entity = entity

   self._sv = self.__saved_variables:get_data()

   if not self._sv._initialized then
      self._sv.region = _radiant.sim.alloc_region3()
      self._sv.height = 0
      self._sv._current_layer = _radiant.sim.alloc_region3()
      self._sv._current_layer_index = 0
      self._sv._initialized = true
      self.__saved_variables:mark_changed()
   else
      self:_restore()
   end

   self._root_terrain_component = radiant._root_entity:add_component('terrain')
end

function WaterComponent:_restore()
end

function WaterComponent:destroy()
end

function WaterComponent:get_region()
   return self._sv.region
end

function WaterComponent:get_water_elevation()
   local entity_location = radiant.entities.get_world_grid_location(self._entity)
   local elevation = entity_location.y + self._sv.height
   return elevation
end

-- TODO: clean up this method
function WaterComponent:_add_water(world_location, volume)
   log:detail('Adding %d water to %s at %s', volume, self._entity, world_location)

   assert(volume >= 0)
   local entity_location = radiant.entities.get_world_grid_location(self._entity)
   local channel_region = Region3()
   local merge_info = nil

   if self._sv.region:get():empty() then
      local region = Region3()
      region:add_point(world_location - entity_location)
      self:_add_to_layer(region)
      volume = self:_subtract_wetting_volume(volume)
   end

   -- fill the channels first so we don't unnecessarily raise and lower layers
   volume = self:_add_water_to_channels(volume)

   -- process and raise layers until we run out of volume
   while volume > 0 do
      local current_layer = self._sv._current_layer:get():translated(entity_location)
      if current_layer:empty() then
         log:warning('Current layer is empty/blocked. Water body may not be able to expand up. Unable to add water.')
         break
      end

      -- TODO: rename to adjacent?
      local edge_region = self:_get_edge_region(current_layer, channel_region)

      if edge_region:empty() then
         -- current layer is bounded, raise the water level until we hit the next layer
         local residual = self:_add_height(volume)
         if residual == volume then
            -- add height was not successful
            break
         end
         volume = residual
      else
         local delta_region = Region3()

         -- grow the region until we run out of volume or edges
         while volume > 0 and not edge_region:empty() do
            local point = edge_region:get_closest_point(world_location)
            local channel = nil

            -- TODO: incrementally update the new edge region
            edge_region:subtract_point(point)

            local target_entity = stonehearth.hydrology:get_water_body(point)
            assert(target_entity ~= self._entity)

            if target_entity then
               if self:_can_merge_with(target_entity) then
                  -- we should save our current region and exit
                  -- a new water entry will process the merged entity
                  merge_info = self:_create_merge_info(self._entity, target_entity)
                  break
               end

               -- Establish a bidirectional link between the two water bodies using two pressure channels.
               -- Confusing, but the source_adjacent_point is inside the target and the target_adjacent_point
               -- is inside the source. This is because the from_location of the channel is defined to be outside
               -- the region of the source water body.
               local source_adjacent_point = point
               local target_adjacent_point = current_layer:get_closest_point(point)
               channel = stonehearth.hydrology:link_pressure_channel(self._entity, source_adjacent_point,
                                                                     target_entity, target_adjacent_point)
            else
               local is_drop = not self:_is_blocked(point - Point3.unit_y)
               if is_drop then
                  -- establish a unidirectional link between the two water bodies using a waterfall channel
                  channel = stonehearth.hydrology:link_waterfall_channel(self._entity, point)
               end
            end

            if channel then
               volume = self:_add_volume_to_channel(channel, volume)
               channel_region:add_point(point)
            else
               -- make this location wet
               delta_region:add_point(point)
               volume = self:_subtract_wetting_volume(volume)
            end
         end

         delta_region:optimize_by_merge()
         delta_region:translate(-entity_location)
         self:_add_to_layer(delta_region)

         if merge_info then
            break
         end
      end
   end

   self.__saved_variables:mark_changed()

   return volume, merge_info
end

function WaterComponent:_remove_water(volume)
   log:detail('Removing %d water from %s', volume, self._entity)

   assert(volume >= 0)
   
   while volume > 0 do
      local residual = self:_remove_height(volume)
      if residual == volume then
         -- remove height was not successful
         break
      end
      volume = residual
   end

   return volume
end

function WaterComponent:_can_merge_with(target)
   local water_elevation = self:get_water_elevation()
   local target_water_component = target:add_component('stonehearth:water')
   local target_water_elevation = target_water_component:get_water_elevation()
   local elevation_delta = water_elevation - target_water_elevation

   -- quick and easy test
   if elevation_delta == 0 then
      return true
   end

   if elevation_delta < 0 then
      return false
   end

   -- only mergable if the top layers are at the same elevation
   if target_water_component._sv._current_layer_index ~= self._sv._current_layer_index then
      return false
   end

   -- if adding a small volume of water through the channel would equalize heights, then allow merge
   -- TODO: consider optimizing repeated calls to O(n) get_area()
   local merge_threshold = 1
   local target_layer_area = target_water_component._sv._current_layer:get():get_area()
   local volume_delta = target_layer_area * elevation_delta
   if volume_delta < 1 then
      return true
   end

   return false
end

function WaterComponent:_create_merge_info(entity1, entity2)
   local merge_info = {
      result = 'merge',
      entity1 = entity1,
      entity2 = entity2
   }
   return merge_info
end

function WaterComponent:_add_water_to_channels(volume)
   local channels = stonehearth.hydrology:get_channels(self._entity)
   local sorted_channels = self:_sort_channels(channels)

   for _, channel in pairs(sorted_channels) do
      local elevation = self:get_water_elevation()
      local channel_height = channel.from_location.y

      if channel_height >= elevation then
         -- we're done becuase channels are sorted by increasing elevation
         break
      end

      volume = self:_add_volume_to_channel(channel, volume)

      if volume <= 0 then
         break
      end
   end

   return volume
end

-- push water into the channels until we max out their capacity
-- TODO: tell hydrology service to mark saved variables as changed after this
function WaterComponent:_fill_channels_to_capacity()
   local channels = stonehearth.hydrology:get_channels(self._entity)
   local sorted_channels = self:_sort_channels(channels)

   for _, channel in pairs(sorted_channels) do
      local elevation = self:get_water_elevation()
      local channel_height = channel.from_location.y

      if channel_height >= elevation then
         -- we're done becuase channels are sorted by increasing elevation
         break
      end

      -- get the flow volume per tick
      local max_flow_volume = stonehearth.hydrology:calculate_channel_flow_rate(channel)
      local unused_volume = max_flow_volume - channel.queued_volume

      if unused_volume > 0 then
         local residual = self:_remove_water(unused_volume)
         if residual == unused_volume then
            -- remove height was not successful
            break
         end
         local flow_volume = unused_volume - residual
         channel.queued_volume = channel.queued_volume + flow_volume

         if flow_volume > 0 then
            log:spam('Added %d to channel for %s at %s', flow_volume, self._entity, channel.from_location)
         end
      end
   end
end

function WaterComponent:_add_volume_to_channel(channel, volume)
   assert(volume >= 0)

   -- get the flow volume per tick
   local max_flow_volume = stonehearth.hydrology:calculate_channel_flow_rate(channel)

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
      log:spam('Added %d to channel for %s at %s', flow_volume, self._entity, channel.from_location)
   end

   return volume
end

-- region in local coordinates
function WaterComponent:_add_to_layer(region)
   self._sv.region:modify(function(cursor)
         cursor:add_region(region)
         cursor:optimize_by_merge()
      end)

   self._sv._current_layer:modify(function(cursor)
         cursor:add_region(region)
         cursor:optimize_by_merge()
      end)
end

-- return value and parameters all in world coordinates
function WaterComponent:_get_edge_region(region, channel_region)
   local world_bounds = self._root_terrain_component:get_bounds()

   -- perform a separable inflation to exclude diagonals
   local inflated = region:inflated(Point3.unit_x) + region:inflated(Point3.unit_z)

   -- subtract the interior region
   local edge_region = inflated - region

   -- remove region blocked by terrain
   local terrain = radiant.terrain.intersect_region(edge_region)
   edge_region:subtract_region(terrain)

   -- remove region blocked by watertight entities
   local collision_region = self:_get_solid_collision_regions(edge_region)
   edge_region:subtract_region(collision_region)

   -- remove channels that we've already processed
   edge_region:subtract_region(channel_region)

   -- remove locations outside the world
   -- TODO: just make these regions channels to nowhere
   edge_region = edge_region:intersect_cube(world_bounds)

   edge_region:optimize_by_merge()

   return edge_region
end

function WaterComponent:_get_solid_collision_regions(region)
   local result = Region3()
   local entities = radiant.terrain.get_entities_in_region(region)
   for _, entity in pairs(entities) do
      local rcs_component = entity:get_component('region_collision_shape')
      if self:_is_watertight(rcs_component) then
         local location = radiant.entities.get_world_grid_location(entity)
         local entity_region = rcs_component:get_region():get():translated(location)
         result:add_region(entity_region)
      end
   end

   return result
end

function WaterComponent:_add_height(volume)
   if volume == 0 then
      return 0
   end
   assert(volume > 0)

   local current_layer = self._sv._current_layer:get()
   local layer_area = current_layer:get_area()
   assert(layer_area > 0)

   local delta = volume / layer_area
   local residual = 0
   local upper_limit = self._sv._current_layer_index + 1

   self._sv.height = self._sv.height + delta

   -- important that this is >= and not > because layer bounds are inclusive on min height but exclusive on max height
   if self._sv.height >= upper_limit then
      residual = (self._sv.height - upper_limit) * layer_area
      self._sv.height = upper_limit
      self:_raise_layer()
   end

   self.__saved_variables:mark_changed()
   
   return residual
end

function WaterComponent:_remove_height(volume)
   if volume == 0 then
      return 0
   end
   assert(volume > 0)

   local lower_limit = self._sv._current_layer_index
   if self._sv.height <= lower_limit then
      self:_lower_layer()
   end

   local current_layer = self._sv._current_layer:get()
   local layer_area = current_layer:get_area()
   assert(layer_area > 0)

   lower_limit = current_layer:get_rect(0).min.y

   local delta = volume / layer_area
   local residual = 0

   self._sv.height = self._sv.height - delta

   if self._sv.height < lower_limit then
      residual = (lower_limit - self._sv.height) * layer_area
      self._sv.height = lower_limit
   end

   self.__saved_variables:mark_changed()

   return residual
end

function WaterComponent:_raise_layer()
   local entity_location = radiant.entities.get_world_grid_location(self._entity)
   local new_layer_index = self._sv._current_layer_index + 1
   log:debug('Raising layer for %s to %d', self._entity, new_layer_index + entity_location.y)

   local current_layer = self._sv._current_layer:get()

   if current_layer:empty() then
      log:warning('Cannot raise layer for %s', self._entity)
      return false
   end

   assert(current_layer:get_rect(0).min.y + 1 == new_layer_index)

   -- convert to world space and raise one level
   local raised_layer = current_layer:translated(entity_location + Point3.unit_y)

   -- subtract any new terrain obstructions
   local intersection = radiant.terrain.intersect_region(raised_layer)
   raised_layer:subtract_region(intersection)
   raised_layer:optimize_by_merge()

   -- back to local space
   raised_layer:translate(-entity_location)

   self._sv.region:modify(function(cursor)
         cursor:add_region(raised_layer)
         cursor:optimize_by_merge()
      end)

   self._sv._current_layer:modify(function(cursor)
         cursor:clear()
         cursor:add_region(raised_layer)
      end)

   self._sv._current_layer_index = new_layer_index

   self.__saved_variables:mark_changed()

   return true
end

function WaterComponent:_lower_layer()
   local entity_location = radiant.entities.get_world_grid_location(self._entity)
   local new_layer_index = self._sv._current_layer_index - 1
   log:debug('Lowering layer for %s to %d', self._entity, new_layer_index + entity_location.y)

   local lowered_layer = self:_get_layer(new_layer_index)

   if lowered_layer:empty() then
      log:warning('Cannot lower layer for %s', self._entity)
      return false
   end

   -- make a copy since the read only reference to current layer will change below
   local top_layer = Region3(self._sv._current_layer:get())

   if not top_layer:empty() then
      assert(top_layer:get_rect(0).min.y - 1 == new_layer_index)
   end

   self._sv.region:modify(function(cursor)
         cursor:subtract_region(top_layer)
      end)

   self._sv._current_layer:modify(function(cursor)
         cursor:clear()
         cursor:add_region(lowered_layer)
      end)

   self._sv._current_layer_index = new_layer_index

   local projected_lower_layer = lowered_layer:translated(Point3.unit_y)
   local residual_top_layer = top_layer - projected_lower_layer

   if not residual_top_layer:empty() then
      -- top layer becomes a new water body with a potentially non-contiguous wet region
      residual_top_layer:optimize_by_merge()
      local parent_location = radiant.entities.get_world_grid_location(self._entity)
      local child_location = residual_top_layer:get_rect(0).min + parent_location
      residual_top_layer:translate(parent_location - child_location)

      local child = stonehearth.hydrology:create_water_body(child_location)
      local child_water_component = child:add_component('stonehearth:water')
      child_water_component:get_region():modify(function(cursor)
            cursor:add_region(residual_top_layer)
         end)

      child_water_component:_recalculate_current_layer()

      log:debug('Top layer from %s becoming new entity %s', self._entity, child)
   end

   self.__saved_variables:mark_changed()

   return true
end

function WaterComponent:_recalculate_current_layer()
   local layer = self:_get_layer(self._sv._current_layer_index)

   self._sv._current_layer:modify(function(cursor)
         cursor:clear()
         cursor:add_region(layer)
      end)

   self.__saved_variables:mark_changed()
end

function WaterComponent:_get_layer(elevation)
   local region = self._sv.region:get()
   local bounds = region:get_bounds()
   bounds.min.y = elevation
   bounds.max.y = elevation + 1

   local layer = region:intersect_cube(bounds)
   layer:optimize_by_merge()
   return layer
end

-- TODO: could optimize by getting the watertight region and testing to see if point is in that set
function WaterComponent:_is_blocked(point)
   if radiant.terrain.is_terrain(point) then
      return true
   end

   local entities = radiant.terrain.get_entities_at_point(point)
   for _, entity in pairs(entities) do
      local rcs_component = entity:get_component('region_collision_shape')
      if self:_is_watertight(rcs_component) then
         return true
      end
   end

   return false
end

function WaterComponent:_is_watertight(region_collision_shape)
   local collision_type = region_collision_shape and region_collision_shape:get_region_collision_type()
   local result = collision_type == _radiant.om.RegionCollisionShape.SOLID
   return result
end

function WaterComponent:_subtract_wetting_volume(volume)
   volume = volume - self._wetting_volume
   volume = math.max(volume, 0)
   return volume
end

function WaterComponent:_sort_channels(channels)
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

return WaterComponent