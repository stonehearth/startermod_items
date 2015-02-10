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
   return self._sv.region:get()
end

function WaterComponent:get_water_elevation()
   local entity_location = radiant.entities.get_world_grid_location(self._entity)
   local elevation = entity_location.y + self._sv.height
   return elevation
end

function WaterComponent:add_water(world_location, volume)
   assert(volume >= 0)
   local entity_location = radiant.entities.get_world_grid_location(self._entity)
   local location = world_location - entity_location
   local delta_region = Region3()
   local new_layer

   if self._sv.region:get():empty() then
      local region = Region3()
      region:add_point(location)
      self:_add_to_layer(region)
      volume = volume - self._wetting_volume
   end

   -- process and raise layers until we run out of volume
   while volume > 0 do
      local edge_region = self:_get_edge_region(self._sv._current_layer:get())
      delta_region:clear()

      -- grow the region until we run out of volume or become bounded
      while volume > 0 and not edge_region:empty() do
         local edge_point = edge_region:get_closest_point(location)
         -- TODO: incrementally update the new edge region
         edge_region:subtract_point(edge_point)

         local world_edge_point = edge_point + entity_location
         local is_drop = not self:_is_blocked(world_edge_point - Point3.unit_y)

         if not is_drop then
            local existing_water_body = stonehearth.hydrology:get_water_body(world_edge_point)
            assert(existing_water_body ~= self._entity)

            if existing_water_body then
               stonehearth.hydrology:merge_water_bodies(self._entity, existing_water_body)
               -- TODO: review this code
               edge_region = self:_get_edge_region(self._sv._current_layer:get())
            else
               delta_region:add_point(edge_point)
               volume = volume - self._wetting_volume
            end
         else
            local from_location = world_edge_point
            local channel = stonehearth.hydrology:get_channel(self._entity, from_location)
            if not channel then
               channel = stonehearth.hydrology:create_waterfall_channel(self._entity, from_location)
            end

            local max_flow_volume = self:_calculate_max_flow_volume(channel.from_location)
            local unused_volume = max_flow_volume - channel.queued_volume
            local flow_volume = math.min(unused_volume, volume)
            channel.queued_volume = channel.queued_volume + flow_volume
            volume = volume - flow_volume
         end
      end

      delta_region:optimize_by_merge()
      self:_add_to_layer(delta_region)

      -- current layer is bounded, raise the water level until we hit the next layer
      if volume > 0 then
         volume, new_layer = self:_add_height(volume)
         if new_layer then
            self:_raise_layer()
         end
      end
   end

   self.__saved_variables:mark_changed()
end

function WaterComponent:_calculate_max_flow_volume(location)
   -- calculate pressure based on a full upper layer
   local reference_elevation = math.floor(self:get_water_elevation()) + 1
   local pressure = reference_elevation - location.y
   local max_flow_volume = pressure * self._pressure_to_flow_rate
   return max_flow_volume
end

function WaterComponent:_raise_layer()
   local entity_location = radiant.entities.get_world_grid_location(self._entity)

   -- convert to world space and raise one level
   local raised_layer = self._sv._current_layer:get():translated(entity_location + Point3.unit_y)

   -- subtract any new terrain obstructions
   local intersection = radiant.terrain.intersect_region(raised_layer)
   raised_layer:subtract_region(intersection)
   raised_layer:optimize_by_merge()

   -- back to local space
   raised_layer:translate(-entity_location)

   self._sv._current_layer:modify(function(cursor)
         cursor:clear()
         cursor:add_region(raised_layer)
      end)

   self._sv.region:modify(function(cursor)
         cursor:add_region(raised_layer)
         cursor:optimize_by_merge()
      end)
end

function WaterComponent:_add_to_layer(region)
   self._sv._current_layer:modify(function(cursor)
         cursor:add_region(region)
         cursor:optimize_by_merge()
      end)

   self._sv.region:modify(function(cursor)
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

   -- remove locations outside the world
   -- TODO: just make these regions channels to nowhere
   edge_region = edge_region:intersect_cube(world_bounds)

   edge_region:optimize_by_merge()

   -- back to local coordinates
   edge_region:translate(-entity_location)

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

function WaterComponent:_add_height(volume)
   local layer_area = self._sv._current_layer:get():get_area()
   local delta = volume / layer_area
   local layer_height_limit = math.floor(self._sv.height) + 1.0
   local residual = 0
   local new_layer = false

   self._sv.height = self._sv.height + delta

   if self._sv.height >= layer_height_limit then
      residual = (self._sv.height - layer_height_limit) * layer_area
      self._sv.height = layer_height_limit
      new_layer = true
   end

   return residual, new_layer
end

return WaterComponent
