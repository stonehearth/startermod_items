local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local Color4 = _radiant.csg.Color4

local WaterRenderer = class()

function WaterRenderer:initialize(render_entity, datastore)
   self._render_entity = render_entity
   self._datastore = datastore
   self._entity = self._render_entity:get_entity()
   self._parent_node = self._render_entity:get_node()
   self._edge_color = Color4(28, 191, 255, 0)
   self._face_color = Color4(28, 191, 255, 192)

   self._datastore_trace = self._datastore:trace_data('rendering water')
      :on_changed(function()
            self:_update()
         end
      )
      :push_object_state()

   self._visible_volume_trace = radiant.events.listen(stonehearth.subterranean_view, 'stonehearth:visible_volume_changed',
                                                      self, self._update)

   stonehearth.selection:set_selectable(self._entity, false)
end

function WaterRenderer:destroy()
   if self._datastore_trace then
      self._datastore_trace:destroy()
      self._datastore_trace = nil
   end

   if self._visible_volume_trace then
      self._visible_volume_trace:destroy()
      self._visible_volume_trace = nil
   end

   self:_destroy_outline_node()
end

function WaterRenderer:_destroy_outline_node()
   if self._outline_node then
      self._outline_node:destroy()
      self._outline_node = nil
   end
end

function WaterRenderer:_update()
   self:_destroy_outline_node()

   local location = radiant.entities.get_world_grid_location(self._entity)
   local data = self._datastore:get_data()
   local height = data.height
   local working_region = data.region:get():translated(location)

   working_region = stonehearth.subterranean_view:intersect_region_with_visible_volume(working_region)
   working_region:optimize_by_merge('water renderer')
   working_region:translate(-location)

   local render_region = Region3()

   for working_cube in working_region:each_cube() do
      local render_cube = Cube3(working_cube)
      if render_cube.max.y > height then
         render_cube.max.y = height

         -- cube must have non-zero thickness for the outline node to generate geometry
         if height - render_cube.min.y < 0.001 then
            render_cube.min.y = height - 0.001
         end
      end

      render_region:add_cube(render_cube)
   end

   self._outline_node = _radiant.client.create_region_outline_node(self._parent_node, render_region, self._edge_color, self._face_color, 'materials/water.material.json')
end

return WaterRenderer
