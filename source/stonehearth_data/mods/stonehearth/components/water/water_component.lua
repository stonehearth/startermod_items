local constants = require 'constants'
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local log = radiant.log.create_logger('water')

local WaterComponent = class()

function WaterComponent:__init()
end

function WaterComponent:initialize(entity, json)
   self._entity = entity

   self._sv = self.__saved_variables:get_data()

   if not self._sv._initialized then
      self._sv.region = nil -- for clarity
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

-- external parties should not modify this region
-- consider making get_region and set_region take non-boxed regions instead
function WaterComponent:get_region()
   return self._sv.region
end

function WaterComponent:set_region(boxed_region, height)
   self._sv.region = boxed_region
   local region = boxed_region:get()
   local current_layer_index

   if region:empty() then
      assert(height == nil or height == 0)
      height = 0
      current_layer_index = 0
   else
      local bounds = region:get_bounds()
      current_layer_index = bounds.max.y - 1

      if height == nil then
         -- use the height of the region
         height = bounds.max.y
      end
   end

   assert(height >= current_layer_index and height <= current_layer_index + 1)
   self._sv.height = height
   self._sv._current_layer_index = current_layer_index
   self:_recalculate_current_layer()

   self.__saved_variables:mark_changed()
end

function WaterComponent:get_water_level()
   local entity_location = radiant.entities.get_world_grid_location(self._entity)
   local water_level = entity_location.y + self._sv.height
   return water_level
end

function WaterComponent:get_current_layer_elevation()
   local entity_location = radiant.entities.get_world_grid_location(self._entity)
   local elevation = entity_location.y + self._sv._current_layer_index
   return elevation
end

-- TODO: clean up this method
function WaterComponent:_add_water(world_location, volume)
   log:detail('Adding %d water to %s at %s', volume, self._entity, world_location)

   assert(volume >= 0)
   local channel_manager = stonehearth.hydrology:get_channel_manager()
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
         log:debug('Current layer for %s is empty/blocked. Water body may not be able to expand up. Unable to add water.', self._entity)
         break
      end

      -- TODO: rename to adjacent?
      local edge_region = self:_get_edge_region(current_layer, channel_region)

      if edge_region:empty() then
         -- current layer is bounded, raise the water level until we hit the next layer
         local residual = self:_add_height(volume)
         if residual == volume then
            log:info('Could not raise water level for %s', self._entity)
            break
         end
         volume = residual
      else
         local delta_region = Region3()

         -- grow the region until we run out of volume or edges
         while volume > 0 and not edge_region:empty() do
            if volume < constants.hydrology.WETTING_VOLUME * 0.5 then
               -- too little volume to wet a block, so just let it evaporate
               volume = 0
               break
            end

            local point = edge_region:get_closest_point(world_location)
            local source_elevation_bias = 0
            local channel = nil

            -- TODO: incrementally update the new edge region
            edge_region:subtract_point(point)

            local target_entity = stonehearth.hydrology:get_water_body(point)
            assert(target_entity ~= self._entity)

            if target_entity then
               if stonehearth.hydrology:can_merge_water_bodies(self._entity, target_entity) then
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
               channel = channel_manager:link_pressure_channel(self._entity, source_adjacent_point,
                                                               target_entity, target_adjacent_point)
               source_elevation_bias = 0
            else
               local is_drop = not self:_is_blocked(point - Point3.unit_y)
               if is_drop then
                  -- establish a unidirectional link between the two water bodies using a waterfall channel
                  channel = channel_manager:link_waterfall_channel(self._entity, point)
                  -- when adding water to the top layer, virtualize the source height because the added water
                  -- has not contributed the height of the layer (which is zero since we are in wetting mode)
                  source_elevation_bias = 1
               end
            end

            if channel then
               volume = channel_manager:add_volume_to_channel(channel, volume, source_elevation_bias)
               channel_region:add_point(point)
            else
               -- make this location wet
               delta_region:add_point(point)
               volume = self:_subtract_wetting_volume(volume)
            end
         end

         delta_region:optimize_by_merge('water:_add_water() delta region')
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

function WaterComponent:_create_merge_info(entity1, entity2)
   local merge_info = {
      result = 'merge',
      entity1 = entity1,
      entity2 = entity2
   }
   return merge_info
end

function WaterComponent:merge_with(mergee, allow_uneven_top_layers)
   self:_merge_regions(self._entity, mergee, allow_uneven_top_layers)

   local channel_manager = stonehearth.hydrology:get_channel_manager()
   channel_manager:merge_channels(self._entity, mergee)
end

-- written as a stateless function
-- mergee should be destroyed soon after this call
function WaterComponent:_merge_regions(master, mergee, allow_uneven_top_layers)
   assert(master ~= mergee)
   local master_location = radiant.entities.get_world_grid_location(master)
   local mergee_location = radiant.entities.get_world_grid_location(mergee)
   log:debug('Merging %s at %s with %s at %s', master, master_location, mergee, mergee_location)

   local master_component = master:add_component('stonehearth:water')
   local mergee_component = mergee:add_component('stonehearth:water')
   local master_layer_elevation = master_component:get_current_layer_elevation()
   local mergee_layer_elevation = mergee_component:get_current_layer_elevation()
   local uneven_merge = allow_uneven_top_layers and master_layer_elevation ~= mergee_layer_elevation
   local translation = mergee_location - master_location   -- translate between local coordinate systems
   local update_layer, new_height, new_index, new_layer_region

   if uneven_merge then
      if mergee_layer_elevation > master_layer_elevation then
         -- adopt the water level of the mergee
         update_layer = true
         new_height = mergee_component:get_water_level() - master_location.y
         new_index = mergee_layer_elevation - master_location.y

         -- clear the master layer since it will be replaced by the mergee kayer below
         master_component._sv._current_layer:modify(function(cursor)
               cursor:clear()
            end)
      else
         -- we're good, just keep the existing layer as it is
         update_layer = false
      end
   else
      -- layers must be at same level
      assert(master_layer_elevation == mergee_layer_elevation)
      update_layer = true

      -- calculate new water level before modifying regions
      local new_water_level = self:_calculate_merged_water_level(master_component, mergee_component)
      new_height = new_water_level - master_location.y
      new_index = master_layer_elevation - master_location.y
   end

   -- merge the current layers
   if update_layer then
      master_component._sv.height = new_height
      master_component._sv._current_layer_index = new_index

      master_component._sv._current_layer:modify(function(cursor)
            local mergee_layer = mergee_component._sv._current_layer:get():translated(translation)
            cursor:add_region(mergee_layer)
         end)
   end

   -- merge the main regions
   master_component._sv.region:modify(function(cursor)
         local mergee_region = mergee_component._sv.region:get():translated(translation)
         cursor:add_region(mergee_region)
      end)

   master_component.__saved_variables:mark_changed()
end

function WaterComponent:_calculate_merged_water_level(master_component, mergee_component)
   local master_layer_elevation = master_component:get_current_layer_elevation()
   local mergee_layer_elevation = mergee_component:get_current_layer_elevation()

   -- layers must be at same level
   assert(master_layer_elevation == mergee_layer_elevation)

   -- get the heights of the current layers
   local master_layer_height = master_component:get_water_level() - master_layer_elevation
   local mergee_layer_height = mergee_component:get_water_level() - mergee_layer_elevation
   assert(master_layer_height <= 1)
   assert(mergee_layer_height <= 1)

   -- TODO: take the residual and add it to the water queue
   -- don't call add_water on the merge to avoid recursive merge logic
   if master_layer_height == 0 or mergee_layer_height == 0 then
      -- yes, assert this again in case the first one is removed
      assert(master_layer_elevation == mergee_layer_elevation)
      return master_layer_elevation
   end

   -- calculate the merged layer height
   local master_layer_area = master_component._sv._current_layer:get():get_area()
   local mergee_layer_area = mergee_component._sv._current_layer:get():get_area()
   local new_layer_area = master_layer_area + mergee_layer_area
   local new_layer_volume = master_layer_area * master_layer_height + mergee_layer_area * mergee_layer_height
   local new_layer_height = new_layer_volume / new_layer_area
   local new_water_level = master_layer_elevation + new_layer_height
   return new_water_level
end

-- TODO: for channels at the same elevation, pick the closest
function WaterComponent:_add_water_to_channels(volume)
   local channel_manager = stonehearth.hydrology:get_channel_manager()
   local water_level = self:get_water_level()

   self:_each_channel_ascending(function(channel)
         local channel_height = channel.from_location.y

         if channel_height > water_level then
            -- we're done becuase channels are sorted by increasing elevation
            return true
         end

         volume = channel_manager:add_volume_to_channel(channel, volume)

         if volume <= 0 then
            -- we're done when we've used up all the water
            return true
         end

         return false
      end)


   return volume
end

-- push water into the channels until we max out their capacity
-- TODO: tell hydrology service to mark saved variables as changed after this
function WaterComponent:_fill_channel_to_capacity(channel)
   local channel_manager = stonehearth.hydrology:get_channel_manager()
   local channel_height = channel.from_location.y
   local water_level = self:get_water_level()

   if channel_height > water_level then
      -- we're done becuase channels are sorted by increasing elevation
      return true
   end

   -- get the flow volume per tick
   local max_flow_volume = channel_manager:calculate_channel_flow_rate(channel)
   local unused_volume = max_flow_volume - channel.queued_volume

   if unused_volume > 0 then
      local residual = self:_remove_water(unused_volume)
      if residual == unused_volume then
         -- we're done becuase no more water is available
         return true
      end
      local flow_volume = unused_volume - residual
      channel.queued_volume = channel.queued_volume + flow_volume

      if flow_volume > 0 then
         log:spam('Added %d to channel for %s at %s', flow_volume, self._entity, channel.from_location)
      end
   end
end

function WaterComponent:update_channel_types()
   local channel_manager = stonehearth.hydrology:get_channel_manager()
   local channels = {}

   self:_each_channel(function(channel)
         table.insert(channels, channel)
      end)

   -- loop over a separate collection since we're modifying the source
   for _, channel in pairs(channels) do
      assert(channel.queued_volume == 0)
      local target_water_component = channel.to_entity:add_component('stonehearth:water')
      local target_water_level = target_water_component:get_water_level()
      local channel_height = channel.from_location.y

      if channel.channel_type == 'pressure' then
         if target_water_level < channel_height then
            -- pressure channel is now a waterfall channel
            channel_manager:link_waterfall_channel(channel.from_entity, channel.from_location, channel.subtype)
         end
      elseif channel.channel_type == 'waterfall' then
         if channel.subtype == 'vertical' then
            -- TODO: support case when to_entity is higher than from_entity
            local source_water_level = self:get_water_level()
            -- recall that vertical waterfall channels have their from_location inside the water body
            if source_water_level <= channel_height then
               -- waterfall dried up and should be removed
               local location = radiant.entities.get_world_grid_location(self._entity)
               self:_remove_from_region(channel.from_location - location)
               channel_manager:remove_channel(channel)
            end

            if target_water_level >= channel_height then
               -- convert to vertical pressure channel
               -- TODO: this violates pressure channel invariants
               channel_manager:link_pressure_channel(channel.from_entity, channel.from_location,
                                                     channel.to_entity, channel.from_location - Point3.unit_y, 'vertical')
            end
         else
            if target_water_level > channel_height then
               -- waterfall is now a pressure channel
               local target_adjacent_point = stonehearth.hydrology:_get_best_channel_adjacent_point(channel.from_entity, channel.from_location)
               channel_manager:link_pressure_channel(channel.from_entity, channel.from_location, channel.to_entity, target_adjacent_point)
            end
         end
      else
         assert(false)
      end
   end
end

function WaterComponent:_each_channel(callback_fn)
   self:_each_channel_impl(callback_fn, false)
end

function WaterComponent:_each_channel_ascending(callback_fn)
   self:_each_channel_impl(callback_fn, true)
end

function WaterComponent:_each_channel_impl(callback_fn, ascending)
   local channel_manager = stonehearth.hydrology:get_channel_manager()
   local channels = channel_manager:get_channels(self._entity)

   if ascending then
      channels = channel_manager:sort_channels_ascending(channels)
   end

   for _, channel in pairs(channels) do
      local stop = callback_fn(channel)
      if stop then
         break
      end
   end
end

function WaterComponent:_get_channels_at_elevation(elevation)
   local channel_manager = stonehearth.hydrology:get_channel_manager()
   local channels = channel_manager:get_channels(self._entity)
   local subset = {}

   for key, channel in pairs(channels) do
      if channel.from_location.y == elevation then
         subset[key] = channel
      end
   end

   return subset
end

-- region in local coordinates
function WaterComponent:_add_to_layer(region)
   self._sv.region:modify(function(cursor)
         cursor:add_region(region)
         cursor:optimize_by_merge('water:_add_to_layer() (region)')
      end)

   self._sv._current_layer:modify(function(cursor)
         cursor:add_region(region)
         cursor:optimize_by_merge('water:_add_to_layer() (current layer)')
      end)

   self.__saved_variables:mark_changed()
end

-- point in local coordinates
function WaterComponent:_remove_from_region(point)
   self._sv.region:modify(function(cursor)
         cursor:subtract_point(point)
         -- optimize_by_merge doesn't usually help for single subtractions
      end)

   self._sv._current_layer:modify(function(cursor)
         cursor:subtract_point(point)
      end)

   self.__saved_variables:mark_changed()
end

-- return value and parameters all in world coordinates
function WaterComponent:_get_edge_region(region, channel_region)
   local world_bounds = self._root_terrain_component:get_bounds()

   -- perform a separable inflation to exclude diagonals
   local inflated = region:inflated(Point3.unit_x) + region:inflated(Point3.unit_z)

   -- subtract the interior region
   local edge_region = inflated - region

   -- remove watertight region
   local watertight_region = stonehearth.hydrology:get_water_tight_region():intersect_region(edge_region)
   edge_region:subtract_region(watertight_region)

   -- remove channels that we've already processed
   edge_region:subtract_region(channel_region)

   -- remove locations outside the world
   edge_region = edge_region:intersect_cube(world_bounds)

   -- TODO: reconsider this call
   edge_region:optimize_by_merge('water:_get_edge_region()')

   return edge_region
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

-- TODO: don't raise into another water body
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

   -- subtract any new obstructions
   local intersection = stonehearth.hydrology:get_water_tight_region():intersect_region(raised_layer)
   raised_layer:subtract_region(intersection)

   -- make sure we don't overlap any other water bodies
   self:_subtract_all_water_regions(raised_layer)

   raised_layer:optimize_by_merge('water:_raise_layer() (raised layer)')

   -- back to local space
   raised_layer:translate(-entity_location)

   self._sv.region:modify(function(cursor)
         cursor:add_region(raised_layer)
         cursor:optimize_by_merge('water:_raise_layer() (updating region)')
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
   local channel_manager = stonehearth.hydrology:get_channel_manager()
   local entity_location = radiant.entities.get_world_grid_location(self._entity)
   local new_layer_index = self._sv._current_layer_index - 1
   log:debug('Lowering layer for %s to %d', self._entity, new_layer_index + entity_location.y)

   local old_layer_elevation = self:get_current_layer_elevation()
   local orphaned_channels = self:_get_channels_at_elevation(old_layer_elevation)

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
      -- we do this to avoid having to create vertical channels to remove each unsupported block
      self:_subtract_unsupported_region(residual_top_layer)
   end

   if not residual_top_layer:empty() then
      -- top layer becomes a new water body with a potentially non-contiguous wet region
      -- TODO: probably need to create an entity for each contiguous set
      residual_top_layer:optimize_by_merge('water:_lower_layer (residual top layer)')
      local parent_location = radiant.entities.get_world_grid_location(self._entity)
      local child_location = residual_top_layer:get_rect(0).min + parent_location
      residual_top_layer:translate(parent_location - child_location)

      local child = stonehearth.hydrology:create_water_body(child_location)
      local child_water_component = child:add_component('stonehearth:water')
      child_water_component:get_region():modify(function(cursor)
            cursor:add_region(residual_top_layer)
         end)

      child_water_component:_recalculate_current_layer()

      -- reparent channels on top layer to child
      channel_manager:reparent_channels(orphaned_channels, child)

      log:debug('Top layer from %s becoming new entity %s', self._entity, child)
   else
      channel_manager:remove_channels(orphaned_channels)
   end

   self.__saved_variables:mark_changed()

   return true
end

function WaterComponent:_subtract_unsupported_region(layer)
   local location = radiant.entities.get_world_grid_location(self._entity)
   local water_tight_region = stonehearth.hydrology:get_water_tight_region()

   -- project one unit down and to world coordinates
   local projected_layer = layer:translated(location - Point3.unit_y)
   local supported_region = water_tight_region:intersect_region(projected_layer)

   -- project one unit up and back to local coordinates
   supported_region:translate(Point3.unit_y - location)
   layer = layer:intersect_region(supported_region)
end

function WaterComponent:_subtract_all_water_regions(region)
   local water_bodies = stonehearth.hydrology:get_water_bodies()

   for id, entity in pairs(water_bodies) do
      local location = radiant.entities.get_world_grid_location(entity)
      local water_component = entity:add_component('stonehearth:water')
      local water_region = water_component:get_region():get():translated(location)
      region:subtract_region(water_region)
   end
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
   layer:optimize_by_merge('water:_get_layer() (elevation:' .. tostring(elevation) .. ')')
   return layer
end

function WaterComponent:_is_blocked(point)
   local result = stonehearth.hydrology:get_water_tight_region():contains_point(point)
   return result
end

function WaterComponent:_is_watertight(region_collision_shape)
   local collision_type = region_collision_shape and region_collision_shape:get_region_collision_type()
   local result = collision_type == _radiant.om.RegionCollisionShape.SOLID
   return result
end

function WaterComponent:_subtract_wetting_volume(volume)
   volume = volume - constants.hydrology.WETTING_VOLUME
   volume = math.max(volume, 0)
   return volume
end

return WaterComponent
