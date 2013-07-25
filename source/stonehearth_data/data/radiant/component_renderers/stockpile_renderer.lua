local StockpileRenderer = class()

function StockpileRenderer:__init(render_entity, json)
   local parent_node = render_entity:get_node()
   self._node = h3dRadiantCreateStockpileNode(parent_node, 'stockpile designation')
   h3dRadiantResizeStockpileNode(self._node, json.size[1], json.size[2])
end

--- xxx: someone call destroy please!!
function StockpileRenderer:__destroy()
   if self._node then
      _radiant.renderer.remove_node(self._node)
      self._node = nil
   end
end

return StockpileRenderer

