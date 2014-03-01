local CityPlanRenderer = class()

function CityPlanRenderer:update(render_entity, data_store)
   self._blueprints = {}
   self._node = render_entity:get_node()
   self._data_store = data_store
   
   self._promise = data_store:trace_data('updating render orders')
   self._promise:on_changed(function ()
         self:_update()
      end)
   self:_update()
end

function CityPlanRenderer:_update()
   local data = self._data_store:get_data()
   
   if not data or not data.blueprints then
      return
   end
   
   -- add new entries.
   for id, entity in pairs(data.blueprints) do
      if not self._blueprints[id] then
         self._blueprints[id] = _client:create_render_entity(self._node, entity)
      end
   end
   
   -- remove deleted entries.
   for id, _ in pairs(self._blueprints) do
      if not data.blueprints[id] then
         self._blueprints[id] = nil
      end
   end
end

function CityPlanRenderer:destroy()
   self._promise:destroy()
end

return CityPlanRenderer
