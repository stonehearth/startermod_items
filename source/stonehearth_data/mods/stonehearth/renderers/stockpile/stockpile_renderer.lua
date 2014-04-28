local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Color4 = _radiant.csg.Color4
local Rect2 = _radiant.csg.Rect2
local Point2 = _radiant.csg.Point2

local StockpileRenderer = class()

function StockpileRenderer:__init()
   self._color = Color4(0, 153, 255, 76)
   self._ui_view_mode = stonehearth.renderer:get_ui_mode()
   self._stockpile_items = {}

   radiant.events.listen(radiant.events, 'stonehearth:ui_mode_changed', self, self._on_ui_mode_changed)
end

function StockpileRenderer:_on_ui_mode_changed(e)
   local mode = stonehearth.renderer:get_ui_mode()
   if self._ui_view_mode ~= mode then
      self._ui_view_mode = mode

      self:_update_item_states(mode, self._stockpile_items)
      self:_update_stockpile_renderer()
   end
end

function StockpileRenderer:_update_stockpile_renderer()
   self._region:modify(function(cursor)
      cursor:clear()
      cursor:add_cube(Rect2(Point2(0, 0), self._size))
   end)
   
   if self:_show_hud() then
      assert(self._hud_renderable == nil)
      self._hud_renderable = _radiant.client.create_designation_node(self._parent_node, self._region:get(), self._color, self._color);
   else
      if self._hud_renderable then
         self._hud_renderable:destroy()
         self._hud_renderable = nil
      end
   end
end

function StockpileRenderer:_update_query_flag(render_entity, mode)
   if self:_show_hud() then
      render_entity:add_query_flag(_radiant.renderer.QueryFlags.UNSELECTABLE)
   else
      render_entity:remove_query_flag(_radiant.renderer.QueryFlags.UNSELECTABLE)
   end
end

function StockpileRenderer:_update_item_states(mode, item_map)
   if item_map == nil then
      return
   end
   for id, item in pairs(item_map) do
      if item:is_valid() then
         local re = _radiant.client.get_render_entity(item)
         if re ~= nil then
            re:set_material_override(self:_mode_to_material_kind(mode))
            self:_update_query_flag(re, mode)
         end
      end
   end
end

function StockpileRenderer:_mode_to_material_kind(mode)
   if mode == 'hud' then
      return 'hud'
   else 
      return 'default'
   end
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

function StockpileRenderer:_diff_and_update_item_states(updated_items)
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

   self:_update_item_states(self._ui_view_mode, added_items)
   self:_update_item_states('', removed_items)
end

function StockpileRenderer:destroy()
   self:_clear()
   radiant.events.unlisten(radiant.events, 'stonehearth:ui_mode_changed', self, self._on_ui_mode_changed)
end

function StockpileRenderer:_update()
   local data = self._savestate:get_data()
   if data and data.size then
      self:_diff_and_update_item_states(data.stocked_items);
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
   if self:_show_hud() then
      assert(self._hud_renderable == nil)
      self._hud_renderable = _radiant.client.create_designation_node(self._parent_node, self._region:get(), self._color, self._color);
   end
   assert(self._dirt_renderable == nil)
   self._dirt_renderable = _radiant.client.create_stockpile_node(self._parent_node, self._region:get(), Color4(55, 49, 26, 48), Color4(55, 49, 26, 64));
end

function StockpileRenderer:_show_hud()
   return self._ui_view_mode == 'hud'
end

function StockpileRenderer:_clear()
   if self._dirt_renderable then
      self._dirt_renderable:destroy()
      self._dirt_renderable = nil
   end

   if self._hud_renderable then
      self._hud_renderable:destroy()
      self._hud_renderable = nil
   end
end


return StockpileRenderer

