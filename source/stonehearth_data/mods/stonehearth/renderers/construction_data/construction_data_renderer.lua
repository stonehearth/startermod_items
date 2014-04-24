local voxel_brush_util = require 'services.server.build.voxel_brush_util'
local Point3 = _radiant.csg.Point3
local ConstructionDataRenderer = class()

function ConstructionDataRenderer:__init(render_entity)
   self._parent_node = render_entity:get_node()
   self._entity = render_entity:get_entity()
   self._render_entity = render_entity 
   -- self:_set_render_mode('rpg')

   self._component_trace = self._entity:trace_components('render construction component')
                              :on_added(function(key, value)
                                    self:_trace_collision_shape()
                                 end)
                              :on_removed(function(key)
                                    self:_trace_collision_shape()
                                 end)
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
   if self._unique_renderable then
      self._unique_renderable:destroy()
      self._unique_renderable = nil
   end
end

function ConstructionDataRenderer:update(render_entity, obj)
   self:_trace_collision_shape()  
   self._construction_data = self._entity:get_component('stonehearth:construction_data'):get_data()

   self:_trace_collision_shape()
   self:_recreate_render_node()
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
                                                self:_recreate_render_node()
                                             end)
      self:_recreate_render_node()                                             
   end
end

function ConstructionDataRenderer:_set_render_mode(mode)
   local listen_to_camera = false
   if self._mode ~= mode then
      self._mode = mode
      -- remove the current mode
      if self._mode == 'rpg' then
         listen_to_camera = true
      end
   end

   if listen_to_camera and not self._listening_to_camera then
      radiant.events.listen(stonehearth.camera, 'stonehearth:camera:update', self, self._update_camera)
      self:_update_camera()
   elseif not listen_to_camera and self._listening_to_camera then
      radiant.events.unlisten(stonehearth.camera, 'stonehearth:camera:update', self, self._update_camera)
   end
end

function ConstructionDataRenderer:_recreate_render_node()
   if self._unique_renderable then
      self._unique_renderable:destroy()
      self._unique_renderable = nil
   end
   
   if self._construction_data and self._collision_shape then   
      local region = self._collision_shape:get_region()
      if region and not region:get():empty() then
         self._unique_renderable = voxel_brush_util.create_construction_data_node(self._parent_node, self._entity, region, self._construction_data)
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
   local cd = self._construction_data
   if self._render_entity and cd then
      local visible = true   
      if self._mode == 'rpg' then
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

