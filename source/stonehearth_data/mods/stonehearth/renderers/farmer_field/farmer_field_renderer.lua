local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Color4 = _radiant.csg.Color4
local Rect2 = _radiant.csg.Rect2
local Point2 = _radiant.csg.Point2

local FarmerFieldRenderer = class()

function FarmerFieldRenderer:__init()
   self._color = Color4(122, 40, 0, 76)
   self._items = {}

   _radiant.call('stonehearth:get_ui_mode'):done(
      function (o)
         self._ui_view_mode = o.mode
      end
   )

   radiant.events.listen(radiant.events, 'stonehearth:ui_mode_changed', function(e)
      if self._ui_view_mode ~= e.mode then
         self._ui_view_mode = e.mode

         self:_update_item_states(e.mode, self._items)
         self:_update_field_renderer()
      end
   end)
end

function FarmerFieldRenderer:_update_field_renderer()
   self._region:modify(function(cursor)
      cursor:clear()
      cursor:add_cube(Rect2(Point2(0, 0), self._size))
   end)
   
   if self:_show_hud() then      
      self._hud_node = _radiant.client.create_designation_node(self._parent_node, self._region:get(), self._color, self._color);
   elseif self._hud_node then
      h3dRemoveNode(self._hud_node)
      self._hud_node = nil
   end
end


function FarmerFieldRenderer:_update_query_flag(render_entity, mode)
   if self:_show_hud() then
      render_entity:add_query_flag(_radiant.renderer.QueryFlags.UNSELECTABLE)
   else
      render_entity:remove_query_flag(_radiant.renderer.QueryFlags.UNSELECTABLE)
   end
end

function FarmerFieldRenderer:_update_item_states(mode, item_map)
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

function FarmerFieldRenderer:_mode_to_material_kind(mode)
   if self:_show_hud() then
      return 'hud'
   else 
      return 'default'
   end
end


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

function FarmerFieldRenderer:_diff_and_update_item_states(updated_items)
   local added_items = {}
   local temp_items = {}

   for id, item in pairs(self._items) do
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

function FarmerFieldRenderer:destroy()
   self:_clear()
end

function FarmerFieldRenderer:_update()
   local data = self._savestate:get_data()
   if data and data.size then
      local contents = {}

      for x=1, data.size.x do
         for y=1, data.size.y do
            local plot = data.contents[x][y].plot
            table.insert(contents, plot)
            
            --- XXX, grab the plant from the plot and add it to the contents.
            -- this method doesn't work because we're client side and can't query
            -- lua components from entities
            --[[
            local dirt_plot_component = plot:get_component('stonehearth:dirt_plot');

            if dirt_plot_component then
               local plant = dirt_plot_component:get_contents()
               if plant then
                  table.insert(contents, plant)
               end
            end
            ]]
         end
      end

      self:_diff_and_update_item_states(contents);
      self._items = contents

      local size = data.size
      if self._size ~= size then
         self._size = size
         self:_regenerate_node()
      end
   end
end

function FarmerFieldRenderer:_regenerate_node()
   self._region:modify(function(cursor)
      cursor:clear()
      cursor:add_cube(Rect2(Point2(0, 0), self._size))
   end)
   
   self:_clear()
   if self:_show_hud() then
      self._hud_node = _radiant.client.create_designation_node(self._parent_node, self._region:get(), self._color, self._color);
   end
   --self._node = _radiant.client.create_stockpile_node(self._parent_node, self._region:get(), Color4(55, 49, 26, 48), Color4(55, 49, 26, 64));
end

function FarmerFieldRenderer:_show_hud()
   return self._ui_view_mode == 'hud'
end


function FarmerFieldRenderer:_clear()
   if self._node then
      h3dRemoveNode(self._node)
      self._node = nil
   end
end

return FarmerFieldRenderer