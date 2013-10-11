local Point3 = _radiant.csg.Point3
local VoxelBrushInfoRenderer = class()

function VoxelBrushInfoRenderer:__init(render_entity, json)
   self._node = render_entity:get_node()
   self._entity = render_entity:get_entity()
   self._info = json
   self._collsion_shape = self._entity:add_component('region_collision_shape')
   if self._collsion_shape then
      self._promise = self._collsion_shape:trace_region('drawing wall')
                                             :on_changed(function ()
                                                self:_update_shape()
                                             end)
      self:_update_shape()
   end
end

function VoxelBrushInfoRenderer:_update_shape()
   if self._model_node then
      self._model_node:destroy()
      self._model_node = nil
   end

   local brush = _radiant.voxel.create_brush(self._info.brush)
   
   if self._info.info_component then
      local render_data = self._entity:get_component_data(self._info.info_component)
      if render_data.normal then
         local normal = Point3(render_data.normal.x, render_data.normal.y, render_data.normal.z)
         brush:set_normal(normal)
      end
   end

   local stencil = self._collsion_shape:get_region():get()
   local model = brush:paint_through_stencil(stencil)
   self._model_node = _radiant.client.create_voxel_render_node(self._node, model)
end

return VoxelBrushInfoRenderer
