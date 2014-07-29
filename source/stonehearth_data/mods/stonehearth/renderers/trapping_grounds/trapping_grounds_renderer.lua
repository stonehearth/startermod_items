local ZoneRenderer = require 'renderers.zone_renderer'

local Color4 = _radiant.csg.Color4

local TrappingGroundsRenderer = class()

function TrappingGroundsRenderer:initialize(render_entity, datastore)
   self._datastore = datastore

   self._zone_renderer = ZoneRenderer(render_entity)
      -- TODO: read these colors from json
      :set_designation_colors(Color4(122, 40, 0, 255), Color4(122, 40, 0, 255))
      :set_ground_colors(Color4(77, 62, 38, 10), Color4(77, 62, 38, 10))

   self._datastore_trace = self._datastore:trace_data('rendering trapping grounds designation')
      :on_changed(
         function()
            self:_update()
         end
      )
      :push_object_state()
end

function TrappingGroundsRenderer:destroy()
   if self._datastore_trace then
      self._datastore_trace:destroy()
      self._datastore_trace = nil
   end

   self._zone_renderer:destroy()
end

function TrappingGroundsRenderer:_update()
   local data = self._datastore:get_data()
   self._zone_renderer:set_size(data.size)
   --self._zone_renderer:set_current_items(data.traps)
end

return TrappingGroundsRenderer
