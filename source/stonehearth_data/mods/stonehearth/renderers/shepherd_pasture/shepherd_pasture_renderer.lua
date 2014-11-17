local ZoneRenderer = require 'renderers.zone_renderer'
local Color4 = _radiant.csg.Color4
local Point2 = _radiant.csg.Point2

local ShepherdPastureRenderer = class()

function ShepherdPastureRenderer:initialize(render_entity, datastore)
   self._datastore = datastore

   self._zone_renderer = ZoneRenderer(render_entity)
      -- TODO: read these colors from json
      :set_designation_colors(Color4(56, 80, 0, 255), Color4(56, 80, 0, 255))
      :set_ground_colors(Color4(77, 62, 38, 10), Color4(77, 62, 38, 10))

   self._datastore_trace = self._datastore:trace_data('rendering shepherd pasture')
      :on_changed(
         function()
            self:_update()
         end
      )
      :push_object_state()
end

function ShepherdPastureRenderer:destroy()
   if self._datastore_trace then
      self._datastore_trace:destroy()
      self._datastore_trace = nil
   end

   self._zone_renderer:destroy()
end


function ShepherdPastureRenderer:_update()
   local data = self._datastore:get_data()
   local size = data.size
   local items = {}

   self._zone_renderer:set_size(Point2(data.size.x, data.size.z))
   self._zone_renderer:set_current_items(items)
end

return ShepherdPastureRenderer