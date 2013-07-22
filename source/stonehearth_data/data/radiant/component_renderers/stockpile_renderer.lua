local StockpileRenderer = class()

function StockpileRenderer:__init(render_entity, json)
   local parent_node = render_entity:get_node()
   self._node = _radiant.renderer.create_designation_node(parent_node, 'stockpile designation')
   _radiant.renderer.resize_designation_node(self._node, json.size[1], json.size[2])
end

--- xxx: someone call destroy please!!
function StockpileRenderer:__destroy()
   if self._node then
      _radiant.renderer.remove_node(self._node)
      self._node = nil
   end
end

return StockpileRenderer

