local CityPlanRenderer = class()

function CityPlanRenderer:__init(render_entity, json)
   self._children = {}
   self._node = render_entity:get_node()
   local renderer = render_entity:get_renderer()
   
   if json.blueprints then
      for _, entity in ipairs(json.blueprints) do
         local child = renderer:create_render_entity(self._node, entity)
         table.insert(self._children, child)
      end
   end
end

function CityPlanRenderer:__destroy()
end

return CityPlanRenderer
