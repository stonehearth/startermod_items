local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local Color4 = _radiant.csg.Color4
local log = radiant.log.create_logger('hilight_service')

local HilightService = class()

function HilightService:initialize()
   self._mouse_x = 0
   self._mouse_y = 0
   self._custom_highlight_fn = nil

   self._frame_trace = _radiant.client.trace_render_frame()
   self._frame_trace:on_frame_start('update hilight service', function(now, alpha, frame_time)
      return self:_on_frame()
   end)

   self._input_capture = stonehearth.input:capture_input()
                              :on_mouse_event(function(e)
                                    return self:_on_mouse_event(e)
                                 end)
                              :on_keyboard_event(function(e)
                                    return self:_on_keyboard_event(e)
                                 end)

   self._enable_terrain_coordinates = radiant.util.get_config('enable_terrain_coordinates', false)
   self._highlight_coordinates = false
   self._last_terrain_brick = nil
end

function HilightService:get_hilighted()
   return self._hilighted
end

function HilightService:add_custom_highlight(custom_highlight_fn)
   self._custom_highlight_fn = custom_highlight_fn
end

function HilightService:_on_frame()
   local hilighted
   local results = _radiant.client.query_scene(self._mouse_x, self._mouse_y)
   local terrain_brick = nil

   for r in results:each_result() do 
      if r.entity:get_id() == 1 then
         terrain_brick = r.brick
      end

      local re = _radiant.client.get_render_entity(r.entity)
      if re and not re:has_query_flag(_radiant.renderer.QueryFlags.UNSELECTABLE) then
         local custom_entity = self._custom_highlight_fn and self._custom_highlight_fn(r)
         hilighted = custom_entity or r.entity
         break
      end
   end

   if hilighted ~= self._hilighted then
      local last_hilghted = self._hilighted
      self._hilighted = hilighted

      _radiant.client.hilight_entity(self._hilighted)

      if last_hilghted and last_hilghted:is_valid() then
         radiant.events.trigger(last_hilghted, 'stonehearth:hilighted_changed')
      end
      if self._hilighted and self._hilighted:is_valid() then
         radiant.events.trigger(self._hilighted, 'stonehearth:hilighted_changed')
      end
   end

   if terrain_brick ~= self._last_terrain_brick then
      self:_update_terrain_coordinates(terrain_brick)
   end

   return false
end

function HilightService:_on_mouse_event(e)
   self._mouse_x = e.x
   self._mouse_y = e.y
   return false
end

function HilightService:_on_keyboard_event(e)
   if e.down then
      if self._enable_terrain_coordinates then
         if e.key == _radiant.client.KeyboardInput.KEY_KP_MULTIPLY then
            self._highlight_coordinates = not self._highlight_coordinates
            self:_update_terrain_coordinates(self._last_terrain_brick)
            return true
         end
      end
   end
   return false
end

function HilightService:_update_terrain_coordinates(terrain_brick)
   self:_destroy_coordinate_nodes()

   if self._highlight_coordinates then
      self:_create_coordinate_nodes(terrain_brick)
   end

   self._last_terrain_brick = terrain_brick
end

function HilightService:_create_coordinate_nodes(brick)
   if not brick then
      return nil
   end

   local text = string.format('%d,%d,%d', brick.x, brick.y, brick.z)
   self._text_node = _radiant.client.create_text_node(1, text)
   self._text_node:set_position(brick + Point3(0, 2, 0))

   local edge_color = Color4(255, 255, 0, 255)
   local face_color = Color4(0, 0, 0, 0)
   local region = Region3()
   region:add_point(Point3.zero)
   self._box_node = _radiant.client.create_region_outline_node(1, region, edge_color, face_color)
   self._box_node:set_position(brick)
end

function HilightService:_destroy_coordinate_nodes()
   if self._text_node then
      self._text_node:destroy()
      self._text_node = nil
   end

   if self._box_node then
      self._box_node:destroy()
      self._box_node = nil
   end
end

return HilightService