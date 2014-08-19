local ZoneRenderer = require 'renderers.zone_renderer'
local Color4 = _radiant.csg.Color4

local NoConstructionZoneRenderer = class()

function NoConstructionZoneRenderer:initialize(render_entity, datastore)
   self._datastore = datastore

   self._zone_renderer = ZoneRenderer(render_entity)
      :set_designation_colors(Color4(32, 32, 32, 0), Color4(128, 128, 128, 255))

   self._datastore_trace = self._datastore:trace_data('rendering no construction zone')
                                                :on_changed(function()
                                                      self:_update()
                                                   end)
                                                :push_object_state()
end

function NoConstructionZoneRenderer:destroy()
   if self._datastore_trace then
      self._datastore_trace:destroy()
      self._datastore_trace = nil
   end
   if self._region_trace then
      self._region_trace:destroy()
      self._region_trace = nil
   end

   self._zone_renderer:destroy()
end

function NoConstructionZoneRenderer:_update()
   local data = self._datastore:get_data()
   if data.region and not self._region then
      self._region = data.region
      self._region_trace = data.region:trace('rendering no construction zone')
                                                :on_changed(function()
                                                      self:_update_shape()
                                                   end)
                                                :push_object_state()
   end
end

function NoConstructionZoneRenderer:_update_shape()
   self._zone_renderer:set_region2(self._region:get())
end

return NoConstructionZoneRenderer
