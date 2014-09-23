local ZoneRenderer = require 'renderers.zone_renderer'
local Point2 = _radiant.csg.Point2
local Color4 = _radiant.csg.Color4

local MiningZoneRenderer = class()

function MiningZoneRenderer:initialize(render_entity, datastore)
   self._datastore = datastore

   self._datastore_trace = self._datastore:trace_data('rendering mining zone designation')
      :on_changed(
         function()
            self:_update()
         end
      )
      :push_object_state()
end

function MiningZoneRenderer:destroy()
   if self._datastore_trace then
      self._datastore_trace:destroy()
      self._datastore_trace = nil
   end
end

function MiningZoneRenderer:_update()
   local data = self._datastore:get_data()
   -- TODO
end

return MiningZoneRenderer
