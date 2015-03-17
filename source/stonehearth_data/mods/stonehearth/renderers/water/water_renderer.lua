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
   self._edge_color = Color4(28, 191, 255, 128)
   self._face_color = Color4(28, 191, 255, 128)

   self._datastore_trace = self._datastore:trace_data('rendering water')
      :on_changed(function()
            self:_update()
         end
      )
      :push_object_state()

   stonehearth.selection:set_selectable(self._entity, false)
end

function WaterRenderer:destroy()
   if self._datastore_trace then
      self._datastore_trace:destroy()
      self._datastore_trace = nil
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

   local data = self._datastore:get_data()
   local region = data.region:get()
   local height = data.height

   local render_region = Region3()
   for cube in region:each_cube() do
      -- TODO: only update height if cube reaches top layer
      cube.max.y = math.min(cube.max.y, height)
      cube.min.y = cube.min.y - 0.001 -- cube must have non-zero thickness for the outline node to generate geometry
      render_region:add_cube(cube)
   end

   self._outline_node = _radiant.client.create_region_outline_node(self._parent_node, render_region, self._edge_color, self._face_color)
end

return WaterRenderer
