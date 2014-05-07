local voxel_brush_util = require 'services.server.build.voxel_brush_util'
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local ConstructionDataRenderer = class()

local INFINITE = 1000000

function ConstructionDataRenderer:__init(render_entity, datastore)
   self._parent_node = render_entity:get_node()
   self._entity = render_entity:get_entity()
   self._render_entity = render_entity 
   self._construction_data = datastore
   
   self._component_trace = self._entity:trace_components('render construction component')
                              :on_added(function(key, value)
                                    self:_trace_collision_shape()
                                 end)
                              :on_removed(function(key)
                                    self:_trace_collision_shape()
                                 end)
   self:_trace_collision_shape()

   radiant.events.listen(radiant, 'stonehearth:building_vision_mode_changed', self, self._on_building_visions_mode_changed)
   self:_on_building_visions_mode_changed()
end

function ConstructionDataRenderer:destroy()
   if self._component_trace then
      self._component_trace:destroy()
      self._component_trace = nil
   end
   if self._collision_shape_trace then
      self._collision_shape_trace:destroy()
      self._collision_shape_trace = nil
   end
   if self._render_node then
      self._render_node:destroy()
      self._render_node = nil
   end
end

function ConstructionDataRenderer:_on_building_visions_mode_changed()
   self:_set_render_mode(stonehearth.renderer:get_building_vision_mode())
end

function ConstructionDataRenderer:_trace_collision_shape()
   local old_collision_shape = self._collision_shape
   self._collision_shape = self._entity:get_component('region_collision_shape')
   
   if self._collision_shape ~= self._old_collision_shape then
      if self._collision_shape_trace then
         self._collision_shape_trace:destroy()
         self._collision_shape_trace = nil
      end
      self._collision_shape_trace = self._collision_shape:trace_region('drawing construction')
                                             :on_changed(function ()
                                                self._rpg_region_dirty = true
                                                self:_recreate_render_node()
                                             end)
      self:_recreate_render_node()
   end
end

function ConstructionDataRenderer:_set_render_mode(mode)
   if self._mode ~= mode then
      self._mode = mode

      local listen_to_camera = self._mode == 'xray'
      if listen_to_camera and not self._listening_to_camera then
         radiant.events.listen(stonehearth.camera, 'stonehearth:camera:update', self, self._update_camera)
         self:_update_camera()
      elseif not listen_to_camera and self._listening_to_camera then
         radiant.events.unlisten(stonehearth.camera, 'stonehearth:camera:update', self, self._update_camera)
      end
      self:_recreate_render_node()
   end
end

function ConstructionDataRenderer:_compute_rpg_region(region)
   if not self._rpg_region then
      self._rpg_region = _radiant.client.alloc_region()
   end
   if self._rpg_region_dirty then
      local building = self._construction_data:get_data().building_entity
      if building then
         local building_origin = radiant.entities.get_world_grid_location(building)
         local local_origin = radiant.entities.get_world_grid_location(self._entity)
         local base = building_origin - local_origin
         local clipper = Cube3(Point3(-INFINITE, base.y,    -INFINITE),
                               Point3( INFINITE, base.y + 1, INFINITE))
         self._rpg_region:modify(function(cursor)
               cursor:copy_region(region:get():clipped(clipper))
            end)
         self._rpg_region_dirty = false
      end
   end
   return self._rpg_region
end

function ConstructionDataRenderer:_get_render_region()
   local region = self._collision_shape:get_region()
   if region then 
      if region:get():empty() then
         region = nil
      elseif self._mode == 'rpg' then
         region = self:_compute_rpg_region(region);
      end
   end
   return region
end

function ConstructionDataRenderer:_recreate_render_node()
   if self._render_node then
      self._render_node:destroy()
      self._render_node = nil
   end
   
   if self._construction_data and self._collision_shape then   
      local render_region = self:_get_render_region()
      if render_region then
         self._render_node = voxel_brush_util.create_construction_data_node(self._parent_node, self._entity, render_region, self._construction_data:get_data())
      end
      self:_update_camera()
   end
end

local function dot_product(p0, p1)
   return (p0.x * p1.x) + (p0.y * p1.y) + (p0.z * p1.z)
end

local function sign(n)
   return n > 0 and 1 or -1   
end

function ConstructionDataRenderer:_update_camera()
   local cd = self._construction_data:get_data()
   if self._render_entity and cd then
      local visible = true   
      if self._mode == 'xray' then
         if cd.normal then
            -- move the camera relative to the mob
            local x0 = stonehearth.camera:get_position() - self._entity:get_component('mob'):get_world_location()

            -- thank you, mr. wolfram.   http://mathworld.wolfram.com/HessianNormalForm.html
            -- The point-plane distance from a point x_0 to a plane (6) is given by the simple equation:
            --
            --    D = n dot x0 + p
            --
            -- p is zero, since we translated x0 into object space.
            visible = dot_product(cd.normal, x0) < 0
         end
      end
      self._render_entity:set_visible(visible)
   end
end

return ConstructionDataRenderer

