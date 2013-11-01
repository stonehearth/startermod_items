local voxel_brush_util = require 'services.build.voxel_brush_util'
local Point3 = _radiant.csg.Point3
local ConstructionDataRenderer = class()

function ConstructionDataRenderer:__init(render_entity, data_store)
   self._parent_node = render_entity:get_node()
   self._entity = render_entity:get_entity()
   self._data_store = data_store
   self._collsion_shape = self._entity:add_component('region_collision_shape')
   if self._collsion_shape then
      self._promise = self._collsion_shape:trace_region('drawing construction')
                                             :on_changed(function ()
                                                self:_update_shape()
                                             end)
      self:_update_shape()
   end
end

function ConstructionDataRenderer:_update_shape()
   if self._node then
      self._node:destroy()
      self._node = nil
   end

   local construction_data = self._data_store:get_data()
   local region = self._collsion_shape:get_region()

   self._node = voxel_brush_util.create_construction_data_node(self._parent_node, self._entity, region, construction_data)
end

return ConstructionDataRenderer
