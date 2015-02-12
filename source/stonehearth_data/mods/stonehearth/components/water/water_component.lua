local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local log = radiant.log.create_logger('water')

local WaterComponent = class()

function WaterComponent:__init()
   -- the volume of water consumed to make a block wet
   self._wetting_volume = 0.25

   -- constant converting pressure to a flow rate per unit cross section
   self._pressure_to_flow_rate = 1
end

function WaterComponent:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()

   if not self._sv._initialized then
      self._sv.region = _radiant.sim.alloc_region3()
      self._sv.height = 0
      self._sv._current_layer = _radiant.sim.alloc_region3()
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
   log:debug('adding %d water to %s at %s', volume, self._entity, world_location)

   assert(volume >= 0)
   local entity_location = radiant.entities.get_world_grid_location(self._entity)
   local location = world_location - entity_location
   local delta_region = Region3()
   local merge_info = nil

   if self._sv.region:get():empty() then
      local region = Region3()
      region:add_point(location)
      self:_add_to_layer(region)
      volume = self:_subtract_wetting_volume(volume)
   end

   -- process and raise layers until we run out of volume
   while volume > 0 do
      local current_layer = self._sv._current_layer:get()
      if current_layer:empty() then
         log:warning('Current layer is empty/blocked. Water body may not be able to expand up. Unable to add water.')
         break
      end

      local edge_region = self:_get_edge_region(current_layer)

      if edge_region:empty() then
         -- current layer is bounded, raise the water level until we hit the next layer
         local residual = self:_add_height(volume)
         if residual == volume then
            -- add height was not successful
            break
         end
         volume = residual
      else
         delta_region:clear()

         -- grow the region until we run out of volume or edges
         while volume > 0 and not edge_region:empty() do
            local point = edge_region:get_closest_point(location)

            -- TODO: incrementally update the new edge region
            edge_region:subtract_point(point)

            local world_point = point + entity_location
            local is_drop = not self:_is_blocked(world_point - Point3.unit_y)

            if not is_drop then
               local existing_water_body = stonehearth.hydrology:get_water_body(world_point)
               assert(existing_water_body ~= self._entity)

               if existing_water_body then
                  -- we should save our current region and exit
                  -- a new water entry will process the merged entity
                  merge_info = {
                     result = 'merge',
                     entity1 = self._entity,
                     entity2 = existing_water_body
                  }
                  break
               else
                  -- make this location wet
                  delta_region:add_point(point)
                  volume = self:_subtract_wetting_volume(volume)
               end
            else
               -- create a waterfall and a channel to the target entity
               local from_location = world_point
               local channel = stonehearth.hydrology:get_channel(self._entity, from_location)
               if not channel then
                  channel = stonehearth.hydrology:create_waterfall_channel(self._entity, from_location)
               end

               volume = self:_add_volume_to_channel(channel, volume)
            end
         end

         delta_region:optimize_by_merge()
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
   log:debug('removing %d water from %s', volume, self._entity)

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

-- push water into the channels until we max out their capacity
-- TODO: tell hydrology service to mark saved variables as changed after this
function WaterComponent:_update_channels(channels)
   local channels = stonehearth.hydrology:get_channels(self._entity)
   local sorted_channels = self:_sort_channels(channels)

   for _, channel in pairs(sorted_channels) do
      log:spam('updating channel for %s at %s', self._entity, channel.from_location)

      local elevation = self:get_water_elevation()
      local channel_height = channel.from_location.y

      if channel_height < elevation then
         local max_flow_volume = self:_calculate_max_flow_volume(channel.from_location)
         local unused_volume = max_flow_volume - channel.queued_volume

         if unused_volume > 0 then
            local residual = self:_remove_water(unused_volume)
            if residual == unused_volume then
               -- remove height was not successful
               break
            end
            local flow_volume = unused_volume - residual
            channel.queued_volume = channel.queued_volume + flow_volume
         end
      end
   end
end

function WaterComponent:_add_volume_to_channel(channel, volume)
   local max_flow_volume = self:_calculate_max_flow_volume(channel.from_location)
   local unused_volume = max_flow_volume - channel.queued_volume
   local flow_volume = math.min(unused_volume, volume)
   channel.queued_volume = channel.queued_volume + flow_volume
   volume = volume - flow_volume
   return volume
end

function WaterComponent:_calculate_max_flow_volume(location)
   -- calculate pressure based on a full upper layer
   local reference_elevation = math.floor(self:get_water_elevation()) + 1
   local pressure = reference_elevation - location.y
   local max_flow_volume = pressure * self._pressure_to_flow_rate
   return max_flow_volume
end

-- returns if the channel capacity is full for the current water level
function WaterComponent:_channel_is_full(channel)
   local max_flow_volume = self:_calculate_max_flow_volume(channel.from_location)
   local full = channel.queued_volume >= max_flow_volume
   return full
end

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

function WaterComponent:_get_edge_region(region)
   local world_bounds = self._root_terrain_component:get_bounds()
   local entity_location = radiant.entities.get_world_grid_location(self._entity)

   -- perform a separable inflation to exclude diagonals
   local inflated = region:inflated(Point3.unit_x) + region:inflated(Point3.unit_z)

   -- subtract the interior region
   local edge_region = inflated - region

   -- to world coordinates
   edge_region:translate(entity_location)

   -- remove region blocked by terrain
   local terrain = radiant.terrain.intersect_region(edge_region)
   edge_region:subtract_region(terrain)

   -- remove region blocked by watertight entities
   local collision_region = self:_get_solid_collision_regions(edge_region)
   edge_region:subtract_region(collision_region)

   -- remove channels that are full
   -- TODO: could optimize by only including channels at this elevation
   local full_channel_region = self:_get_full_channel_region()
   edge_region:subtract_region(full_channel_region)

   -- remove locations outside the world
   -- TODO: just make these regions channels to nowhere
   edge_region = edge_region:intersect_cube(world_bounds)

   edge_region:optimize_by_merge()

   -- back to local coordinates
   edge_region:translate(-entity_location)

   return edge_region
end

function WaterComponent:_get_full_channel_region()
   local channels = stonehearth.hydrology:get_channels(self._entity)
   local result = Region3()

   for _, channel in pairs(channels) do
      if self:_channel_is_full(channel) then
         result:add_point(channel.from_location)
      end
   end

   return result
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
   local upper_limit = current_layer:get_rect(0).max.y
   assert(upper_limit == math.floor(self._sv.height + 1))

   self._sv.height = self._sv.height + delta

   -- important that this is >= and not >
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

   local lower_limit = math.floor(self._sv.height)
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

   -- important that this is < and not <=
   if self._sv.height < lower_limit then
      residual = (lower_limit - self._sv.height) * layer_area
      self._sv.height = lower_limit
   end

   self.__saved_variables:mark_changed()

   return residual
end

function WaterComponent:_raise_layer()
   log:debug('raising layer for %s', self._entity)

   local current_layer = self._sv._current_layer:get()

   if current_layer:empty() then
      log:warning('cannot raise layer for %s', self._entity)
      return false
   end

   local entity_location = radiant.entities.get_world_grid_location(self._entity)

   -- convert to world space and raise one level
   local raised_layer = current_layer:translated(entity_location + Point3.unit_y)

   local layer_elevation = (raised_layer:get_rect(0).min + entity_location).y
   log:debug('new layer for %s at %d', self._entity, layer_elevation)

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

   self.__saved_variables:mark_changed()

   return true
end

function WaterComponent:_lower_layer()
   log:debug('lowering layer for %s', self._entity)

   local bounds = self._sv.region:get():get_bounds()
   local new_upper_bound = bounds.max.y - 1
   bounds.max.y = new_upper_bound
   bounds.min.y = new_upper_bound - 1

   local lower_layer = self._sv.region:get():intersect_cube(bounds)
   lower_layer:optimize_by_merge()

   if lower_layer:empty() then
      log:warning('cannot lower layer for %s', self._entity)
      return false
   end

   local entity_location = radiant.entities.get_world_grid_location(self._entity)
   local layer_elevation = (lower_layer:get_rect(0).min + entity_location).y
   log:debug('new layer for %s at %d', self._entity, layer_elevation)

   -- make a copy since the read only reference to current layer will change below
   local top_layer = Region3(self._sv._current_layer:get())

   self._sv.region:modify(function(cursor)
         cursor:subtract_region(top_layer)
      end)

   self._sv._current_layer:modify(function(cursor)
         cursor:clear()
         cursor:add_region(lower_layer)
      end)

   local projected_lower_layer = lower_layer:translated(Point3.unit_y)
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

      log:debug('top layer from %s becoming new entity %s', self._entity, child)
   end

   self.__saved_variables:mark_changed()

   return true
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
