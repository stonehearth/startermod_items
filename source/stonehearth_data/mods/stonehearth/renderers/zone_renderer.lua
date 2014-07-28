local Point2 = _radiant.csg.Point2
local Rect2 = _radiant.csg.Rect2
local Region2 = _radiant.csg.Region2
local Color4 = _radiant.csg.Color4

local ZoneRenderer = class()

function ZoneRenderer:__init(render_entity)
   local default_color = Color4(128, 128, 128, 128)

   self._items = {}
   self._designation_color_interior = default_color
   self._designation_color_border = default_color
   self._ground_color_interior = default_color
   self._ground_color_border = default_color
   self._parent_node = render_entity:get_node()

   self:set_size(Point2.zero)
   self:_on_ui_mode_changed()

   radiant.events.listen(radiant, 'stonehearth:ui_mode_changed', self, self._on_ui_mode_changed)
end

function ZoneRenderer:destroy()
   radiant.events.unlisten(radiant, 'stonehearth:ui_mode_changed', self, self._on_ui_mode_changed)

   -- take all items out of ghost mode
   self:_update_item_states(self._items, false)

   self:_destroy_designation_node()
   self:_destroy_ground_node()
end

function ZoneRenderer:set_size(size)
   if size ~= self._size then
      self._size = size
      self._region = Region2(Rect2(Point2.zero, self._size))

      self:_regenerate_designation_node()      
      self:_regenerate_ground_node()
   end

   return self
end

function ZoneRenderer:set_designation_colors(interior_color, border_color)
   self._designation_color_interior = interior_color
   self._designation_color_border = border_color
   self:_regenerate_designation_node()

   return self
end

function ZoneRenderer:set_ground_colors(interior_color, border_color)
   self._ground_color_interior = interior_color
   self._ground_color_border = border_color
   self:_regenerate_ground_node()

   return self
end

function ZoneRenderer:set_current_items(current_items)
   local added_items = {}
   local temp_items = {}
   local removed_items

   for id, item in pairs(self._items) do
      temp_items[id] = item
   end

   for id, item in pairs(current_items) do
      if temp_items[id] then
         temp_items[id] = nil
      else
         added_items[id] = item
      end
   end

   -- temp items now contains the list of removed items
   removed_items = temp_items

   self:_update_item_states(added_items, self:_in_hud_mode())
   self:_update_item_states(removed_items, false)

   -- make a copy of current items for the next update
   self._items = {}
   for id, item in pairs(current_items) do
      self._items[id] = item
   end

   return self
end

function ZoneRenderer:_update_item_states(items, ghost_mode)
   for id, item in pairs(items) do
      if item:is_valid() then
         local render_entity = _radiant.client.get_render_entity(item)
         if render_entity then
            self:_set_ghost_mode(render_entity, ghost_mode)
         end
      end
   end
end

function ZoneRenderer:_set_ghost_mode(render_entity, ghost_mode)
   local material_kind

   if ghost_mode then
      material_kind = 'hud'
      render_entity:add_query_flag(_radiant.renderer.QueryFlags.UNSELECTABLE)
   else
      material_kind = 'default'
      render_entity:remove_query_flag(_radiant.renderer.QueryFlags.UNSELECTABLE)
   end

   local material = render_entity:get_material_path(material_kind)           
   render_entity:set_material_override(material)
end

function ZoneRenderer:_on_ui_mode_changed()
   local mode = stonehearth.renderer:get_ui_mode()

   if self._ui_view_mode ~= mode then
      self._ui_view_mode = mode

      self:_update_item_states(self._items, self:_in_hud_mode())
      self:_regenerate_designation_node()
      -- no need to regenerate ground node
   end
end

function ZoneRenderer:_in_hud_mode()
   return self._ui_view_mode == 'hud'
end

function ZoneRenderer:_destroy_designation_node()
   if self._designation_node then
      self._designation_node:destroy()
      self._designation_node = nil
   end
end

function ZoneRenderer:_regenerate_designation_node()
   self:_destroy_designation_node()

   if self:_in_hud_mode() then
      self._designation_node = _radiant.client.create_designation_node(self._parent_node, self._region,
                               self._designation_color_interior, self._designation_color_border);
   end
end

function ZoneRenderer:_destroy_ground_node()
   if self._ground_node then
      self._ground_node:destroy()
      self._ground_node = nil
   end
end

function ZoneRenderer:_regenerate_ground_node() 
   self:_destroy_ground_node()

   self._ground_node = _radiant.client.create_stockpile_node(self._parent_node, self._region,
                       self._ground_color_interior, self._ground_color_border);
end

return ZoneRenderer
