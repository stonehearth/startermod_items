local ZoneRenderer = require 'renderers.zone_renderer'

local Color4 = _radiant.csg.Color4

local FarmerFieldRenderer = class()

function FarmerFieldRenderer:initialize(render_entity, datastore)
   self._datastore = datastore

   self._zone_renderer = ZoneRenderer(render_entity)
      -- TODO: read these colors from json
      :set_designation_colors(Color4(122, 40, 0, 76), Color4(122, 40, 0, 76))
      :set_ground_colors(Color4(77, 62, 38, 10), Color4(77, 62, 38, 10))

   self._datastore_trace = self._datastore:trace_data('rendering farmer field designation')
      :on_changed(
         function()
            self:_update()
         end
      )
      :push_object_state()
end

function FarmerFieldRenderer:destroy()
   if self._datastore_trace then
      self._datastore_trace:destroy()
      self._datastore_trace = nil
   end

   self._zone_renderer:destroy()
end

function FarmerFieldRenderer:_update()
   local data = self._datastore:get_data()
   local size = data.size
   local items = {}

   for _, row in pairs(data.contents) do
      for _, dirt_plot_entity in pairs(row) do
         if dirt_plot_entity:is_valid() then
            -- add the dirt plot entity to the list
            items[dirt_plot_entity:get_id()] = dirt_plot_entity
            -- crops on the dirt plot are not currently put in ghost mode
         end
      end
   end

   self._zone_renderer:set_size(size)
   self._zone_renderer:set_current_items(items)
end

return FarmerFieldRenderer
