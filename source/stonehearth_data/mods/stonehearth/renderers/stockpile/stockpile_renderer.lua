local StockpileRenderer = class()

function StockpileRenderer:__init(render_entity, data_store)
   local parent_node = render_entity:get_node()
   self._size = { 0, 0 }   
   self._data_store = data_store
   self._node = h3dRadiantCreateStockpileNode(parent_node, 'stockpile designation')
   h3dSetNodeParamStr(self._node, H3DNodeParams.NameStr, 'stockpile designation node')
   local name = h3dGetNodeParamStr(self._node, H3DNodeParams.NameStr)
   self._promise = data_store:trace('rendering stockpile designation')
   self._promise:on_changed(function()
         self:_update()
      end)
   self:_update()      
end

--- xxx: someone call destroy please!!
function StockpileRenderer:_update()
   local data = self._data_store:get_data()
   if data and data.size then
      local size = data.size
      if self._size[1] ~= size[1] or self._size[2] ~= size[2] then
         self._size = { size[1], size[2] }
         h3dRadiantResizeStockpileNode(self._node, size[1], size[2])
      end
   end
end

function StockpileRenderer:destroy()
   if self._node then
      _radiant.renderer.remove_node(self._node)
      self._node = nil
   end
end

return StockpileRenderer

