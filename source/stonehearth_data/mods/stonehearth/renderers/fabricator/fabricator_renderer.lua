local voxel_brush_util = require 'services.build.voxel_brush_util'
local Point3 = _radiant.csg.Point3
local FabricatorRenderer = class()

function FabricatorRenderer:__init(render_entity, data_store)
   self._parent_node = render_entity:get_node()
   self._entity = render_entity:get_entity()

   self._data = data_store:get_data()
   self._data_store = data_store
   self._ds_promise = data_store:trace_data('rendering a fabrication')
                        :on_changed(function()
                           self:_update()
                        end)

   self._destination = self._entity:get_component('destination')
   if self._destination then
      self._cs_promise = self._destination:trace_region('drawing fabricator')
                                                :on_changed(function ()
                                                   self:_update()
                                                end)
      self:_update()
   end
   self:_update()
end

function FabricatorRenderer:_update()
   if self._node then
      self._node:destroy()
      self._node = nil
   end

   local blueprint = self._data.blueprint
   local construction_data = blueprint:get_component_data('stonehearth:construction_data')
   local region = self._destination:get_region()

   self._node = voxel_brush_util.create_construction_data_node(self._parent_node, self._entity, region, construction_data, 'blueprint')
end

function FabricatorRenderer:destroy()
end

return FabricatorRenderer