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
      self._sv._top_layer = _radiant.sim.alloc_region3()
      self._sv._top_layer_index = 0
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
   local top_layer_index

   if region:empty() then
      assert(height == nil or height == 0)
      height = 0
      top_layer_index = 0
   else
      local bounds = region:get_bounds()
      top_layer_index = bounds.max.y - 1

      if height == nil then
         -- use the height of the region
         height = bounds.max.y
      end
   end

   assert(height >= top_layer_index and height <= top_layer_index + 1)
   self._sv.height = height
   self._sv._top_layer_index = top_layer_index
   self:_recalculate_top_layer()

   self.__saved_variables:mark_changed()
end

function WaterComponent:get_water_level()
   local entity_location = radiant.entities.get_world_grid_location(self._entity)
   local water_level = entity_location.y + self._sv.height
   return water_level
end

function WaterComponent:get_top_layer_elevation()
   local entity_location = radiant.entities.get_world_grid_location(self._entity)
   local elevation = entity_location.y + self._sv._top_layer_index
   return elevation
end

function WaterComponent:top_layer_in_wetting_mode()
   self:_normalize()
   -- don't compare water level, because we can convert a non-integer value of self._sv.height
   -- to an integer value via precision loss
   local result = self._sv.height == self._sv._top_layer_index
   return result
end

-- TODO: clean up this method
function WaterComponent:_add_water(add_location, volume)
   assert(volume >= 0)
   if volume == 0 then
      return
   end
   log:detail('Adding %f water to %s at %s', volume, self._entity, add_location)

   local entity_location = radiant.entities.get_world_grid_location(self._entity)
   local channel_region = self:_get_channel_region()
   local info = {}

   if self._sv.region:get():empty() then
      local region = Region3()
      region:add_point(add_location - entity_location)
      self:_add_to_layer(region)
      volume = self:_subtract_wetting_volume(volume)
   end

   -- fill the channels first so we don't unnecessarily raise and lower layers
   volume = self:_add_water_to_channels(volume)

   -- process and raise layers until we run out of volume
   while volume > 0 do
      local top_layer = self._sv._top_layer:get():translated(entity_location)
      if top_layer:empty() then
         log:debug('Top layer for %s is empty/blocked. Water body may not be able to expand up. Unable to add water.', self._entity)
         info = {
            result = 'bounded',
            reason = 'top layer empty'
         }
         break
      end

      -- TODO: rename to adjacent?
      local edge_region = self:_get_edge_region(top_layer, channel_region)

      if edge_region:empty() then
         -- top layer is contained, raise the water level until we hit the next layer
         local residual = self:_add_height(volume)
         if residual == volume then
            log:info('Could not raise water level for %s', self._entity)
            info = {
               result = 'bounded',
               reason = 'could not raise water level'
            }
            break
         end
         volume = residual
      else
         if self:_allow_grow_region(add_location, edge_region) then
            volume, info = self:_grow_region(volume, add_location, top_layer, edge_region, channel_region)
         else
            log:detail('Discarded water for %s because edge area exceeeded', self._entity)
            volume = 0
            info = {
               result = 'discarded',
               reason = 'edge area exceeded'
            }
         end

         if info.result then
            break
         end
      end
   end

   self.__saved_variables:mark_changed()

   return volume, info
end

function WaterComponent:_allow_grow_region(add_location, edge_region)
   if edge_region:get_area() <= constants.hydrology.EDGE_AREA_LIMIT then
      return true
   end

   -- player created mining regions are allowed to grow unbounded
   local mined_region = stonehearth.mining:get_mined_region()
   if mined_region:contains(add_location) then
      return true
   end

   return false
end

function WaterComponent:_grow_region(volume, add_location, top_layer, edge_region, channel_region)
   local channel_manager = stonehearth.hydrology:get_channel_manager()
   local entity_location = radiant.entities.get_world_grid_location(self._entity)
   local add_region = Region3()
   local info = {}

   -- grow the region until we run out of volume or edges
   while volume > 0 and not edge_region:empty() do
      if volume < constants.hydrology.WETTING_VOLUME * 0.5 then
         -- too little volume to wet a block, so just let it evaporate
         volume = 0
         break
      end

      local point = edge_region:get_closest_point(add_location)
      local channel = nil

      -- TODO: incrementally update the new edge region
      edge_region:subtract_point(point)

      local target_entity = stonehearth.hydrology:get_water_body(point)
      assert(target_entity ~= self._entity)

      if target_entity then
         if stonehearth.hydrology:can_merge_water_bodies(self._entity, target_entity) then
            -- We should save our current region and exit. Caller will add the remaining volume on the merged entity.
            info = {
               result = 'merge',
               entity1 = self._entity,
               entity2 = target_entity
            }
            break
         end

         -- Establish a bidirectional link between the two water bodies using two pressure channels.
         local from_location = top_layer:get_closest_point(point)
         local to_location = point
         channel = channel_manager:link_pressure_channel(self._entity, target_entity, from_location, to_location)
      else
         local is_drop = not self:_is_blocked(point - Point3.unit_y)
         if is_drop then
            -- establish a unidirectional link between the two water bodies using a waterfall channel
            local source_location = top_layer:get_closest_point(point)
            local waterfall_top = point
            channel = channel_manager:link_waterfall_channel(self._entity, source_location, waterfall_top)
         end
      end

      if channel then
         volume = self:_add_water_to_channel(channel, volume)
         channel_region:add_point(point)
      else
         -- make this location wet
         add_region:add_point(point)
         volume = self:_subtract_wetting_volume(volume)
      end
   end

   add_region:optimize_by_merge('water:_grow_region')
   add_region:translate(-entity_location)
   self:_add_to_layer(add_region)

   return volume, info
end

function WaterComponent:_remove_water(volume)
   assert(volume >= 0)
   if volume == 0 then
      return
   end
   log:detail('Removing %f water from %s', volume, self._entity)
   
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

function WaterComponent:_normalize()
   assert(self._sv.height >= self._sv._top_layer_index)
   assert(self._sv.height <= self._sv._top_layer_index + 1)

   -- don't compare water level, because we can convert a non-integer value of self._sv.height
   -- to an integer value via precision loss
   if self._sv.height == self._sv._top_layer_index + 1 then
      self:_raise_layer()
   end
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

   master_component:_normalize()
   mergee_component:_normalize()

   local master_layer_elevation = master_component:get_top_layer_elevation()
   local mergee_layer_elevation = mergee_component:get_top_layer_elevation()

   local translation = mergee_location - master_location   -- translate between local coordinate systems
   local update_layer, new_height, new_index, new_layer_region

   local is_uneven_merge = self:_is_uneven_merge(master_component, mergee_component)
   local do_uneven_merge = allow_uneven_top_layers and is_uneven_merge

   if do_uneven_merge then
      if mergee_layer_elevation > master_layer_elevation then
         -- adopt the water level of the mergee
         update_layer = true
         new_height = mergee_component:get_water_level() - master_location.y
         new_index = mergee_layer_elevation - master_location.y

         -- clear the master layer since it will be replaced by the mergee kayer below
         master_component._sv._top_layer:modify(function(cursor)
               cursor:clear()
            end)
      else
         -- we're good, just keep the existing layer as it is
         update_layer = false
      end
   else
      -- layers must be at same level
      if master_layer_elevation ~= mergee_layer_elevation then
         master_component:_normalize()
         mergee_component:_normalize()
         assert(master_layer_elevation == mergee_layer_elevation)
      end

      update_layer = true

      -- calculate new water level before modifying regions
      local new_water_level = self:_calculate_merged_water_level(master_component, mergee_component)
      new_height = new_water_level - master_location.y
      new_index = master_layer_elevation - master_location.y
   end

   -- merge the top layers
   if update_layer then
      master_component._sv.height = new_height
      master_component._sv._top_layer_index = new_index

      master_component._sv._top_layer:modify(function(cursor)
            local mergee_layer = mergee_component._sv._top_layer:get():translated(translation)
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

function WaterComponent:_is_uneven_merge(master_component, mergee_component)
   local master_layer_elevation = master_component:get_top_layer_elevation()
   local mergee_layer_elevation = mergee_component:get_top_layer_elevation()

   if master_layer_elevation ~= mergee_layer_elevation then
      return true
   end

   -- it's uneven if one of the layers cannot grow but the other can
   local master_bounded = master_component._sv._top_layer:get():empty()
   local mergee_bounded = mergee_component._sv._top_layer:get():empty()
   local result = master_bounded ~= mergee_bounded
   return result
end

function WaterComponent:_calculate_merged_water_level(master_component, mergee_component)
   local master_layer_elevation = master_component:get_top_layer_elevation()
   local mergee_layer_elevation = mergee_component:get_top_layer_elevation()

   -- layers must be at same level
   assert(master_layer_elevation == mergee_layer_elevation)

   -- get the heights of the top layers
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
   local master_layer_area = master_component._sv._top_layer:get():get_area()
   local mergee_layer_area = mergee_component._sv._top_layer:get():get_area()
   local new_layer_area = master_layer_area + mergee_layer_area
   local new_layer_volume = master_layer_area * master_layer_height + mergee_layer_area * mergee_layer_height
   local new_layer_height = new_layer_volume / new_layer_area
   local new_water_level = master_layer_elevation + new_layer_height
   return new_water_level
end

function WaterComponent:_add_water_to_channel(channel, volume)
   local water_level = self:get_water_level()
   local source_elevation = channel.source_location.y

   if source_elevation > water_level then
      -- channel is too high to be used
      return volume
   end

   local top_layer_elevation = self:get_top_layer_elevation()
   local is_top_layer_waterfall = channel.channel_type == 'waterfall' and source_elevation == top_layer_elevation

   -- when adding water to the top layer, virtualize the source height because the added water
   -- has not contributed the height of the layer (particularly if we're in wetting mode)
   local source_elevation_bias = is_top_layer_waterfall and 1 or 0

   local channel_manager = stonehearth.hydrology:get_channel_manager()
   volume = channel_manager:add_volume_to_channel(channel, volume, source_elevation_bias)

   return volume
end

-- TODO: for channels at the same elevation, pick the closest
function WaterComponent:_add_water_to_channels(volume)
   local channel_manager = stonehearth.hydrology:get_channel_manager()
   local water_level = self:get_water_level()
   local top_layer_elevation = self:get_top_layer_elevation()

   self:_each_channel_ascending(function(channel)
         local source_elevation = channel.source_location.y
         if source_elevation > water_level then
            -- we're done becuase channels are sorted by increasing elevation
            return true
         end

         volume = self:_add_water_to_channel(channel, volume)
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
function WaterComponent:fill_channel_from_water_region(channel)
   local channel_manager = stonehearth.hydrology:get_channel_manager()
   local source_elevation = channel.source_location.y
   local water_level = self:get_water_level()

   if source_elevation > water_level then
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
         log:spam('Added %f to channel for %s at %s', flow_volume, self._entity, channel.channel_entrance)
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
      local source_elevation = channel.source_location.y

      if channel.subtype == 'vertical' then
         -- TODO: support case when to_entity is higher than from_entity
         local source_water_level = self:get_water_level()
         -- recall that vertical waterfall channels have their from_location inside the water body
         if source_water_level <= source_elevation then
            -- waterfall dried up and should be removed
            local location = radiant.entities.get_world_grid_location(self._entity)
            self:_remove_from_region(channel.source_location - location)
            channel_manager:remove_channel(channel)
            channel = nil
         end
      end

      if channel then
         if channel.channel_type == 'pressure' then
            if target_water_level < source_elevation then
               -- pressure channel is now a waterfall channel
               channel = channel_manager:convert_to_waterfall_channel(channel)
               -- TODO: remove paired channel from current channels list so we don't have to check it
               -- note that keeping it in the list should be harmless
            end
         elseif channel.channel_type == 'waterfall' then
            if target_water_level >= source_elevation then
               -- waterfall is now a pressure channel
               channel = channel_manager:convert_to_pressure_channel(channel)
            end
         else
            -- unknown channel_type
            assert(false)
         end
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

function WaterComponent:_get_channel_region()
   local region = Region3()

   self:_each_channel(function(channel)
         region:add_point(channel.channel_entrance)
      end)

   region:optimize_by_merge('water:_get_channel_region()')
   return region
end

function WaterComponent:_get_channels_at_elevation(elevation)
   local channel_manager = stonehearth.hydrology:get_channel_manager()
   local channels = channel_manager:get_channels(self._entity)
   local subset = {}

   for key, channel in pairs(channels) do
      if channel.source_location.y == elevation then
         subset[key] = channel
      end
   end

   return subset
end

-- region must exist within the current y bounds of the existing water region
function WaterComponent:add_to_region(region)
   self._sv.region:modify(function(cursor)
         cursor:add_region(region)
         cursor:optimize_by_merge('water:add_to_region()')
      end)

   local bounds = region:get_bounds()

   -- we could support this, but it is not a currently valid use case
   assert(bounds.min.y >= 0)
   assert(bounds.max.y - 1 <= self._sv._top_layer_index)

   if bounds.max.y - 1 == self._sv._top_layer_index then
      self:_recalculate_top_layer()
   end
end

-- region in local coordinates
function WaterComponent:_add_to_layer(region)
   if region:empty() then
      return
   end

   self._sv.region:modify(function(cursor)
         cursor:add_region(region)
         cursor:optimize_by_merge('water:_add_to_layer() (region)')
      end)

   self._sv._top_layer:modify(function(cursor)
         cursor:add_region(region)
         cursor:optimize_by_merge('water:_add_to_layer() (top layer)')
      end)

   self:_update_destination()

   self.__saved_variables:mark_changed()
end

-- point in local coordinates
function WaterComponent:_remove_from_region(point)
   self._sv.region:modify(function(cursor)
         cursor:subtract_point(point)
         -- optimize_by_merge doesn't usually help for single subtractions
      end)

   self._sv._top_layer:modify(function(cursor)
         cursor:subtract_point(point)
      end)

   self.__saved_variables:mark_changed()
end

function WaterComponent:_update_destination()
   local destination_component = self._entity:add_component('destination')
   destination_component:get_region():modify(function(cursor)
         cursor:clear()
         cursor:copy_region(self._sv._top_layer:get())
      end)

   -- TODO: update adjacent
end

-- return value and parameters all in world coordinates
function WaterComponent:_get_edge_region(region, channel_region)
   local world_bounds = self._root_terrain_component:get_bounds()

   -- perform a separable inflation to exclude diagonals
   local edge_region = region:inflated(Point3.unit_x)
   edge_region:add_region(region:inflated(Point3.unit_z))

   -- subtract the interior region
   edge_region:subtract_region(region)

   -- remove watertight region
   local watertight_region = stonehearth.hydrology:get_water_tight_region():intersect_region(edge_region)
   edge_region:subtract_region(watertight_region)

   -- remove channels that we've already processed
   edge_region:subtract_region(channel_region)

   -- remove locations outside the world
   edge_region = edge_region:intersect_cube(world_bounds)

   edge_region:optimize_by_merge('water:_get_edge_region()')

   return edge_region
end

function WaterComponent:_add_height(volume)
   if volume == 0 then
      return 0
   end
   assert(volume > 0)

   local top_layer = self._sv._top_layer:get()
   local layer_area = top_layer:get_area()
   assert(layer_area > 0)

   local delta = volume / layer_area
   local residual = 0
   local upper_limit = self._sv._top_layer_index + 1

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

   local lower_limit = self._sv._top_layer_index
   if self._sv.height <= lower_limit then
      self:_lower_layer()
      lower_limit = self._sv._top_layer_index
   end

   local top_layer = self._sv._top_layer:get()
   local layer_area = top_layer:get_area()
   assert(layer_area > 0)

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
   local new_layer_index = self._sv._top_layer_index + 1
   log:debug('Raising layer for %s to %d', self._entity, new_layer_index + entity_location.y)

   local top_layer = self._sv._top_layer:get()

   if top_layer:empty() then
      log:warning('Cannot raise layer for %s', self._entity)
      return false
   end

   assert(top_layer:get_rect(0).min.y + 1 == new_layer_index)

   -- convert to world space and raise one level
   local raised_layer = top_layer:translated(entity_location + Point3.unit_y)

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

   self._sv._top_layer:modify(function(cursor)
         cursor:clear()
         cursor:add_region(raised_layer)
      end)

   self._sv._top_layer_index = new_layer_index

   self.__saved_variables:mark_changed()

   return true
end

function WaterComponent:_lower_layer()
   local channel_manager = stonehearth.hydrology:get_channel_manager()
   local entity_location = radiant.entities.get_world_grid_location(self._entity)
   local new_layer_index = self._sv._top_layer_index - 1
   log:debug('Lowering layer for %s to %d', self._entity, new_layer_index + entity_location.y)

   local old_layer_elevation = self:get_top_layer_elevation()
   local orphaned_channels = self:_get_channels_at_elevation(old_layer_elevation)

   local lowered_layer = self:_get_layer(new_layer_index)

   if lowered_layer:empty() then
      log:warning('Cannot lower layer for %s', self._entity)
      return false
   end

   -- make a copy since the read only reference to top layer will change below
   local top_layer = Region3(self._sv._top_layer:get())

   if not top_layer:empty() then
      assert(top_layer:get_rect(0).min.y - 1 == new_layer_index)
   end

   self._sv.region:modify(function(cursor)
         cursor:subtract_region(top_layer)
      end)

   self._sv._top_layer:modify(function(cursor)
         cursor:clear()
         cursor:add_region(lowered_layer)
      end)

   self._sv._top_layer_index = new_layer_index

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
      -- to world_coordinates
      residual_top_layer:translate(entity_location)

      local child = stonehearth.hydrology:create_water_body_with_region(residual_top_layer, 0)

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

function WaterComponent:_recalculate_top_layer()
   local layer = self:_get_layer(self._sv._top_layer_index)

   self._sv._top_layer:modify(function(cursor)
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
