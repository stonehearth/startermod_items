local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Color3 = _radiant.csg.Color3
local Rect2 = _radiant.csg.Rect2
local Point2 = _radiant.csg.Point2

local StockpileRenderer = class()

function StockpileRenderer:__init()
   self._color = Color3(0, 153, 255)
   self._ui_view_mode = 'normal'
   self._stockpile_items = {}

   radiant.events.listen(radiant.events, 'stonehearth:ui_mode_changed', function(e)
      if self._ui_view_mode ~= e.mode then
         if e.mode == 'normal' or e.mode == 'zones' then
            self._ui_view_mode = e.mode

            self:_update_item_renderers(e.mode, self._stockpile_items)
            self:_update_stockpile_renderer(e.mode)
         end
      end
   end)
end

function StockpileRenderer:_update_stockpile_renderer(mode)
   -- TODO: Here, we decide what we want the stockpile to look like.
   self:_regenerate_node()
end

function StockpileRenderer:_update_item_renderers(mode, item_map)
   if item_map == nil then
      return
   end
   for id, item in pairs(item_map) do
      if item:is_valid() then
         local f = _radiant.client.get_render_entity(item)
         f:set_material_override(self:_mode_to_material_kind(mode))
      end
   end
end

function StockpileRenderer:_mode_to_material_kind(mode)
   if mode == 'normal' then
      return 'default'
   end
   return mode
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

function StockpileRenderer:_diff_and_update_item_renderers(updated_items)
   local added_items = {}
   local temp_items = {}

   for id, item in pairs(self._stockpile_items) do
      temp_items[id] = item
   end

   for id, item in pairs(updated_items) do
      if temp_items[id] == nil then
         added_items[id] = item
      end
      temp_items[id] = nil
   end

   -- Rename for clarity!
   local removed_items = temp_items

   self:_update_item_renderers(self._ui_view_mode, added_items)
   self:_update_item_renderers('', removed_items)
end

--- xxx: someone call destroy please!!
function StockpileRenderer:destroy()
   self:_clear()
end

function StockpileRenderer:_update()
   local data = self._savestate:get_data()
   if data and data.size then
      self:_diff_and_update_item_renderers(data.stocked_items);
      self._stockpile_items = data.stocked_items

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

