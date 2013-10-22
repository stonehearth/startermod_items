local Point3 = _radiant.csg.Point3

local FabricatorRenderer = class()

function FabricatorRenderer:__init(render_entity, data_store)
   self._node = render_entity:get_node()
   self._entity = render_entity:get_entity()

   self._data = data_store:get_data()
   self._data_store = data_store
   self._ds_promise = data_store:trace('rendering a fabrication')
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
   if self._model_node then
      self._model_node:destroy()
      self._model_node = nil
   end
   if self._model_outline_node then
      self._model_outline_node:destroy()
      self._model_outline_node = nil
   end


   local entity = self._data.blueprint
   self._info = radiant.entities.get_entity_data(entity, 'stonehearth:voxel_brush_info')

   -- start common function
   local region = self._destination:get_region()
   if region and region:get() then
      local brush = _radiant.voxel.create_brush(self._info.brush)
      local meshing_mode = ''
          
      if self._info.info_component then
         local render_data = entity:get_component_data(self._info.info_component)
         if render_data then
            if render_data.normal then
               local normal = Point3(render_data.normal.x, render_data.normal.y, render_data.normal.z)
               brush:set_normal(normal)
            end
            if render_data.paint_mode == 'blueprint' then
               meshing_mode = 'blueprint'
               brush:set_paint_mode(_radiant.voxel.QubicleBrush.Opaque)
            end            
         end
      end
      
      local material = ''
      local render_info = self._entity:get_component('render_info') -- YES.  self._entity!!
      if render_info then
         material = render_info:get_material()
      end

      local stencil = region:get()
      local model = brush:paint_through_stencil(stencil)
      if meshing_mode == 'blueprint' then
         self._model_outline_node = _radiant.client.create_voxel_render_node(self._node, model, 'blueprint', material)
      end
      self._model_node = _radiant.client.create_voxel_render_node(self._node, model, '', material)
   end
end

function FabricatorRenderer:destroy()
end

return FabricatorRenderer