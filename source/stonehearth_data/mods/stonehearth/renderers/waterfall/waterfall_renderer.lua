local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local Color4 = _radiant.csg.Color4

local WaterfallRenderer = class()

function WaterfallRenderer:initialize(render_entity, datastore)
   self._render_entity = render_entity
   self._datastore = datastore
   self._entity = self._render_entity:get_entity()
   self._parent_node = self._render_entity:get_node()
   self._edge_color = Color4(28, 191, 255, 128)
   self._face_color = Color4(28, 191, 255, 128)

   self._datastore_trace = self._datastore:trace_data('rendering waterfall')
      :on_changed(function()
            self:_update()
         end
      )
      :push_object_state()

   stonehearth.selection:set_selectable(self._entity, false)
end

function WaterfallRenderer:destroy()
   if self._datastore_trace then
      self._datastore_trace:destroy()
      self._datastore_trace = nil
   end

   self:_destroy_outline_node()
end

function WaterfallRenderer:_destroy_outline_node()
   if self._outline_node then
      self._outline_node:destroy()
      self._outline_node = nil
   end
end

function WaterfallRenderer:_update()
   self:_destroy_outline_node()

   local data = self._datastore:get_data()
   local region = data.region:get()

   self._outline_node = _radiant.client.create_region_outline_node(self._parent_node, region, self._edge_color, self._face_color)
end

return WaterfallRenderer
