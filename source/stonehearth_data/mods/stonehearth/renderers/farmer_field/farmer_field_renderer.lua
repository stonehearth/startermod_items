local RendererHelpers = require 'renderers.renderer_helpers'

local Color4 = _radiant.csg.Color4
local Rect2 = _radiant.csg.Rect2
local Point2 = _radiant.csg.Point2

local FarmerFieldRenderer = class()

function FarmerFieldRenderer:initialize(render_entity, datastore)
   self._datastore = datastore
   self._color = Color4(122, 40, 0, 76)
   self._items = {}

   radiant.events.listen(radiant, 'stonehearth:ui_mode_changed', self, self._ui_mode_changed)
   self._parent_node = render_entity:get_node()
   self._size = { 0, 0 }   
   self._region = _radiant.client.alloc_region2()
   self._ui_view_mode = stonehearth.renderer:get_ui_mode()

   self._datastore_trace = self._datastore:trace_data('rendering farmer field designation')
   self._datastore_trace:on_changed(function()
         self:_update()
      end)
   self:_update()
end

function FarmerFieldRenderer:destroy()
   radiant.events.unlisten(radiant, 'stonehearth:ui_mode_changed', self, self._ui_mode_changed)
   
   if self._datastore_trace then
      self._datastore_trace:destroy()
      self._datastore_trace = nil
   end
end

function FarmerFieldRenderer:_ui_mode_changed()   
   local mode = stonehearth.renderer:get_ui_mode()
   if self._ui_view_mode ~= mode then
      self._ui_view_mode = mode

      self:_update_item_states(mode, self._items)
      self:_update_field_renderer()
   end
end

function FarmerFieldRenderer:_update_field_renderer()
   self._region:modify(function(cursor)
      cursor:clear()
      cursor:add_cube(Rect2(Point2.zero, self._size))
   end)
   
   if self:_show_hud() then
      assert(self._render_node == nil)
      self._render_node = _radiant.client.create_designation_node(self._parent_node, self._region:get(), self._color, self._color);
   elseif self._render_node then
      self._render_node:destroy()
      self._render_node = nil
   end
end

function FarmerFieldRenderer:_update_item_states(mode, item_map)
   if item_map == nil then
      return
   end
   for id, item in pairs(item_map) do
      if item:is_valid() then
         local render_entity = _radiant.client.create_render_entity(item)
         RendererHelpers.set_ghost_mode(render_entity, mode == 'hud')
      end
   end
end

function FarmerFieldRenderer:_diff_and_update_item_states(updated_items)
   local added_items = {}
   local temp_items = {}

   for _, item in pairs(self._items) do
      temp_items[item:get_id()] = item
   end

   for _, item in pairs(updated_items) do
      local id = item:get_id()
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

function FarmerFieldRenderer:destroy()
   self:_clear()
   radiant.events.unlisten(radiant, 'stonehearth:ui_mode_changed', self, self._ui_mode_changed)
end

function FarmerFieldRenderer:_update()
   local data = self._datastore:get_data()
   if data then
      local contents = {}

      for _, row in pairs(data.contents) do
         for _, plot in pairs(row) do
            table.insert(contents, plot)
         end
      end

      self:_diff_and_update_item_states(contents);
      self._items = contents


      if self._size ~= data.size then
         self._size = data.size
         self:_regenerate_node()
      end
   end
end

function FarmerFieldRenderer:_regenerate_node()
   self._region:modify(function(cursor)
      cursor:clear()
      cursor:add_cube(Rect2(Point2.zero, self._size))
   end)
   
   self:_clear()
   if self:_show_hud() then
      assert(self._render_node == nil)
      self._render_node = _radiant.client.create_designation_node(self._parent_node, self._region:get(), self._color, self._color);
   end
end

function FarmerFieldRenderer:_show_hud()
   return self._ui_view_mode == 'hud'
end

function FarmerFieldRenderer:_clear()
   if self._render_node then
      self._render_node:destroy()
      self._render_node = nil
   end
end

return FarmerFieldRenderer