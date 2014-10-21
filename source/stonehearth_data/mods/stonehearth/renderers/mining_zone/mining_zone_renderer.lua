local ZoneRenderer = require 'renderers.zone_renderer'
local Point2 = _radiant.csg.Point2
local Color4 = _radiant.csg.Color4

local MiningZoneRenderer = class()

function MiningZoneRenderer:initialize(render_entity, datastore)
   self._parent_node = render_entity:get_node()
   self._datastore = datastore
   self._outline_color = Color4(255, 255, 0, 0)

   self._ui_view_mode = stonehearth.renderer:get_ui_mode()
   self._ui_mode_listener = radiant.events.listen(radiant, 'stonehearth:ui_mode_changed', self, self._on_ui_mode_changed)

   self._datastore_trace = self._datastore:trace_data('rendering mining zone designation')
      :on_changed(function()
            self:_update()
         end
      )
      :push_object_state()
end

function MiningZoneRenderer:destroy()
   if self._ui_mode_listener then
      self._ui_mode_listener:destroy()
      self._ui_mode_listener = nil
   end

   if self._datastore_trace then
      self._datastore_trace:destroy()
      self._datastore_trace = nil
   end

   self:_destroy_outline_node()
end

function MiningZoneRenderer:_destroy_outline_node()
   if self._render_node then
      self._render_node:destroy()
      self._render_node = nil
   end
end

function MiningZoneRenderer:_update()
   self:_destroy_outline_node()

   if not self:_in_hud_mode() then
      return
   end

   local data = self._datastore:get_data()
   local region = data.region:get()

   self._render_node = _radiant.client.create_region_outline_node(self._parent_node, region, self._outline_color)
end

function MiningZoneRenderer:_on_ui_mode_changed()
   local mode = stonehearth.renderer:get_ui_mode()

   if self._ui_view_mode ~= mode then
      self._ui_view_mode = mode

      self:_update()
   end
end

function MiningZoneRenderer:_in_hud_mode()
   return self._ui_view_mode == 'hud'
end

return MiningZoneRenderer
