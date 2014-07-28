local ZoneRenderer = require 'renderers.zone_renderer'

local Color4 = _radiant.csg.Color4

local StockpileRenderer = class()

function StockpileRenderer:initialize(render_entity, datastore)
   self._datastore = datastore

   self._zone_renderer = ZoneRenderer(render_entity)
      -- TODO: read these colors from json
      :set_designation_colors(Color4(0, 153, 255, 76), Color4(0, 153, 255, 76))
      :set_ground_colors(Color4(55, 49, 26, 48), Color4(55, 49, 26, 64))

   self._datastore_trace = self._datastore:trace_data('rendering stockpile designation')
      :on_changed(
         function()
            self:_update()
         end
      )
      :push_object_state()
end

function StockpileRenderer:destroy()
   if self._datastore_trace then
      self._datastore_trace:destroy()
      self._datastore_trace = nil
   end

   self._zone_renderer:destroy()
end

function StockpileRenderer:_update()
   local data = self._datastore:get_data()
   self._zone_renderer:set_size(data.size)
   self._zone_renderer:set_current_items(data.stocked_items)
end

return StockpileRenderer
