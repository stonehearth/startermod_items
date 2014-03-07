local voxel_brush_util = require 'services.build.voxel_brush_util'
local Point3 = _radiant.csg.Point3
local ConstructionDataRenderer = class()

function ConstructionDataRenderer:__init(render_entity)
   self._parent_node = render_entity:get_node()
   self._entity = render_entity:get_entity()
   
   self._collision_shape = self._entity:add_component('region_collision_shape')
   if self._collision_shape then
      self._promise = self._collision_shape:trace_region('drawing construction')
                                             :on_changed(function ()
                                                self:_render()
                                             end)
   end
end

function ConstructionDataRenderer:destroy()
   if self._promise then
      self._promise:destroy()
      self._promise = nil
   end
   if self._node then
      self._node:destroy()
      self._node = nil
   end
end

function ConstructionDataRenderer:update(render_entity, obj)
   self._construction_data = obj:get_data()
   self:_render()
end

function ConstructionDataRenderer:_render()
   if self._node then
      self._node:destroy()
      self._node = nil
   end
   if self._construction_data and self._collision_shape then
      local region = self._collision_shape:get_region()
      if region then
         self._node = voxel_brush_util.create_construction_data_node(self._parent_node, self._entity, region, self._construction_data)
      end
   end
end

return ConstructionDataRenderer
