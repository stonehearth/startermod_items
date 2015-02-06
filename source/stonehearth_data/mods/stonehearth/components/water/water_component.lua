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

-- TODO: schedule add_water through the hydrology service
function WaterComponent:add_water(world_location, volume)
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

   while volume > 0 do
      local flows = stonehearth.hydrology:get_flows(self._entity)
      if flows then
         for _, flow in pairs(flows) do
            -- TODO: flow volume not accurate with multiple passes
            local flow_volume = math.min(volume, 1)
            stonehearth.hydrology:queue_water(flow.to_entity, flow.to_location, flow_volume)
            volume = volume - flow_volume
         end
         if volume <= 0 then
            break
         end
      end

      local edge_region = self:_get_edge_region(self._sv._current_layer:get())
      if edge_region:empty() then
         volume, new_layer = self:_add_height(volume)
         if new_layer then
            self:_raise_layer()
         end
      else
         delta_region:clear()

         while not edge_region:empty() do
            local edge_point = edge_region:get_closest_point(location)
            -- TODO: incrementally update the new edge region
            -- especially important when added flows are not in the center of the layer
            edge_region:subtract_point(edge_point)

            local world_edge_point = edge_point + entity_location
            local is_drop = not radiant.terrain.is_terrain(world_edge_point - Point3.unit_y)

            if not is_drop then
               local existing_water_body = stonehearth.hydrology:get_water_body(world_edge_point)
               if existing_water_body then
                  stonehearth.hydrology:merge_water_bodies(self._entity, existing_water_body)
                  -- TODO: edge_region needs to be updated!
               else
                  delta_region:add_point(edge_point)
                  volume = volume - self._wetting_volume
                  if volume <= 0 then
                     break
                  end
               end
            else
               local from_location = world_edge_point
               local flow = self:_create_waterfall_flow(from_location)

               -- TODO: calculate flow throughput
               local flow_volume = math.min(volume, 1)
               stonehearth.hydrology:queue_water(flow.to_entity, flow.to_location, flow_volume)
               volume = volume - flow_volume
            end
         end

         delta_region:optimize_by_merge()

         self:_add_to_layer(delta_region)
      end
   end

   self.__saved_variables:mark_changed()
end

function WaterComponent:_create_waterfall_flow(from_location)
   local to_location = radiant.terrain.get_point_on_terrain(from_location)
   local to_entity = stonehearth.hydrology:get_water_body(to_location)
   if not to_entity then
      to_entity = stonehearth.hydrology:create_water_body(to_location)
   end

   local waterfall = stonehearth.hydrology:create_waterfall(from_location, to_location)
   local flow = stonehearth.hydrology:add_flow(self._entity, from_location, to_entity, to_location, waterfall)
   return flow
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

   -- remove blocked locations
   local intersection = radiant.terrain.intersect_region(edge_region)
   edge_region:subtract_region(intersection)

   -- remove locations outside the world
   edge_region = edge_region:intersect_cube(world_bounds)

   -- remove flow points
   local flow_points = self:_get_flow_points()
   edge_region:subtract_region(flow_points)

   edge_region:optimize_by_merge()

   -- back to local coordinates
   edge_region:translate(-entity_location)

   return edge_region
end

function WaterComponent:_get_flow_points()
   local region = Region3()
   local flows = stonehearth.hydrology:get_flows(self._entity)
   for _, flow in pairs(flows) do
      region:add_point(flow.from_location)
   end
   return region
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
