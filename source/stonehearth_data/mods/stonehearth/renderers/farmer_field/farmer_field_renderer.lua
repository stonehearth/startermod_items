--[[
   Draws a player visible rectangle around the place designated as a field
]]
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Color3 = _radiant.csg.Color3
local Rect2 = _radiant.csg.Rect2
local Point2 = _radiant.csg.Point2

local FarmerFieldRenderer = class()

-- Review Q: It's the same name as _update except without the _? 
-- Can we get a more descriptive name?
function FarmerFieldRenderer:update(render_entity, saved_variables)
   self._parent_node = render_entity:get_node()
   self._size = { 0, 0 }   
   self._savestate = saved_variables
   self._region = _radiant.client.alloc_region2()

   self._promise = saved_variables:trace_data('rendering farmer field designation')
   self._promise:on_changed(function()
         self:_update()
      end)
   self:_update()
end

function FarmerFieldRenderer:destroy()
   self:_clear()
end

function FarmerFieldRenderer:_update()
   local data = self._savestate:get_data()
   if data and data.size then
      local size = data.size
      if self._size[1] ~= size[1] or self._size[2] ~= size[2] then
         self._size = { size[1], size[2] }
         self._region:modify(function(cursor)
            cursor:clear()
            cursor:add_cube(Rect2(Point2(0, 0), Point2(size[1], size[2])))
         end)
         
         self:_clear()
         self._node = _radiant.client.create_designation_node(self._parent_node, self._region:get(), Color3(230, 201, 54), Color3(230, 201, 54));
      end
   end
end

function FarmerFieldRenderer:_clear()
   if self._node then
      h3dRemoveNode(self._node)
      self._node = nil
   end
end

return FarmerFieldRenderer