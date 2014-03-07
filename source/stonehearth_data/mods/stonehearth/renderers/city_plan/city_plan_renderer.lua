local CityPlanRenderer = class()

function CityPlanRenderer:__init(render_entity)
   self._blueprints = {}
   self._parent_node = render_entity:get_node()
end

function CityPlanRenderer:update(render_entity, data)
   if not data or not data.blueprints then
      return
   end
  
   -- add new entries.
   for id, entity in pairs(data.blueprints) do
      if not self._blueprints[id] then
         self._blueprints[id] = _client:create_render_entity(self._parent_node, entity)
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
end

return CityPlanRenderer
