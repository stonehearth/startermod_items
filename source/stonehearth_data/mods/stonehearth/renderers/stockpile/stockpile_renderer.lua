local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Color3 = _radiant.csg.Color3
local Rect2 = _radiant.csg.Rect2
local Point2 = _radiant.csg.Point2

local StockpileRenderer = class()

function StockpileRenderer:__init(render_entity, data_store)
   self._parent_node = render_entity:get_node()
   self._size = { 0, 0 }   
   self._data_store = data_store
   self._region = _radiant.client.alloc_region2()

   self._promise = data_store:trace_data('rendering stockpile designation')
   self._promise:on_changed(function()
         self:_update()
      end)
   self:_update()      
end

--- xxx: someone call destroy please!!
function StockpileRenderer:destroy()
   self:_clear()
end

function StockpileRenderer:_update()
   local data = self._data_store:get_data()
   if data and data.size then
      local size = data.size
      if self._size ~= size then
         self._size = size
         self._region:modify(function(cursor)
            cursor:clear()
            cursor:add_cube(Rect2(Point2(0, 0), size))
         end)
         
         self:_clear()
         self._node = _radiant.client.create_designation_node(self._parent_node, self._region:get(), Color3(0, 153, 255), Color3(0, 153, 255));
      end
   end
end

function StockpileRenderer:_clear()
   if self._node then
      h3dRemoveNode(self._node)
      self._node = nil
   end
end


return StockpileRenderer

