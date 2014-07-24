local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Color4 = _radiant.csg.Color4
local Rect2 = _radiant.csg.Rect2
local Point2 = _radiant.csg.Point2
local Region2 = _radiant.csg.Region2

local StockpileRenderer = class()

function StockpileRenderer:initialize(render_entity, datastore)
   self._datastore = datastore
   self._color = Color4(0, 153, 255, 76)
   self._stockpile_items = {}

   radiant.events.listen(radiant, 'stonehearth:ui_mode_changed', self, self._on_ui_mode_changed)

   self._parent_node = render_entity:get_node()     
   self._trace = self._datastore:trace_data('rendering stockpile designation')
                                          :on_changed(function()
                                                self:_update()
                                             end)
   self:_update()
   self:_on_ui_mode_changed()
end

function StockpileRenderer:destroy()
   self:_update_item_states('default', self._sv.stocked_items)

   if self._trace then
      self._trace:destroy()
      self._trace = nil
   end
   
   if self._dirt_renderable then
      self._dirt_renderable:destroy()
      self._dirt_renderable = nil
   end

   if self._hud_renderable then
      self._hud_renderable:destroy()
      self._hud_renderable = nil
   end
   radiant.events.unlisten(radiant, 'stonehearth:ui_mode_changed', self, self._on_ui_mode_changed)
end

function StockpileRenderer:_on_ui_mode_changed(e)
   local mode = stonehearth.renderer:get_ui_mode()
   if self._ui_view_mode ~= mode then
      self._ui_view_mode = mode

      self:_update_item_states(mode, self._stockpile_items)
      self:_regenerate_hud_node()
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
            local kind = self:_mode_to_material_kind(mode)
            local material = re:get_material_path(kind)
            re:set_material_override(material)

            if mode == 'hud' then
               re:add_query_flag(_radiant.renderer.QueryFlags.UNSELECTABLE)
            else
               re:remove_query_flag(_radiant.renderer.QueryFlags.UNSELECTABLE)
            end
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

function StockpileRenderer:_update()
   self._sv = self._datastore:get_data()
   if self._sv.size then
      self:_diff_and_update_item_states(self._sv.stocked_items);
      self._stockpile_items = self._sv.stocked_items

      if not self._region or self._region:get_bounds().max ~= self._sv.size then
         self._region = Region2(Rect2(Point2(0, 0), self._sv.size))

         self:_regenerate_hud_node()
         self:_regenerate_dirt_node()
      end
   end
end

function StockpileRenderer:_regenerate_hud_node() 
   if self._hud_renderable then
      self._hud_renderable:destroy()
      self._hud_renderable = nil
   end
   if self:_show_hud() then
      self._hud_renderable = _radiant.client.create_designation_node(self._parent_node, self._region, self._color, self._color);
   end
end

function StockpileRenderer:_regenerate_dirt_node() 
   if self._dirt_renderable then
      self._dirt_renderable:destroy()
      self._dirt_renderable = nil
   end
   self._dirt_renderable = _radiant.client.create_stockpile_node(self._parent_node, self._region, Color4(55, 49, 26, 48), Color4(55, 49, 26, 64));
end

function StockpileRenderer:_show_hud()
   return self._ui_view_mode == 'hud'
end

return StockpileRenderer
