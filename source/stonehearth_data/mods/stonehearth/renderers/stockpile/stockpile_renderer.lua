local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Color3 = _radiant.csg.Color3
local Rect2 = _radiant.csg.Rect2
local Point2 = _radiant.csg.Point2

local StockpileRenderer = class()

function StockpileRenderer:__init()
   self._ui_view_mode = 'normal'
   self._color = Color3(0, 153, 255)

   radiant.events.listen(radiant.events, 'stonehearth:ui_mode_changed', function(e)
      if self._ui_view_mode ~= e.mode then
         self._color = Color3(255 - self._color.r, self._color.g, self._color.b)
         self:_regenerate_node()
         self._ui_view_mode = e.mode
      end
   end)
end

function StockpileRenderer:update(render_entity, saved_variables)
   self._parent_node = render_entity:get_node()
   self._size = { 0, 0 }   
   self._savestate = saved_variables
   self._region = _radiant.client.alloc_region2()

   if self._promise then
      self._promise:destroy()
      self._promise = nil
   end
   
   self._promise = saved_variables:trace_data('rendering stockpile designation')
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
   local data = self._savestate:get_data()
   if data and data.size then
      local size = data.size
      if self._size ~= size then
         self._size = size
         self:_regenerate_node()
      end
   end
end

function StockpileRenderer:_regenerate_node()
   self._region:modify(function(cursor)
      cursor:clear()
      cursor:add_cube(Rect2(Point2(0, 0), self._size))
   end)
   
   self:_clear()
   self._node = _radiant.client.create_designation_node(self._parent_node, self._region:get(), self._color, self._color);
end

function StockpileRenderer:_clear()
   if self._node then
      h3dRemoveNode(self._node)
      self._node = nil
   end
end


return StockpileRenderer

