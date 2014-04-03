local Point3 = _radiant.csg.Point3
local input_constants = require('constants').input

local UnitControlService = class()

function UnitControlService:initialize()
   self._input_capture = stonehearth.input:capture_input()
                              :on_mouse_event(function(e)
                                   return self:_on_mouse_input(e)
                                 end)
end

function UnitControlService:_on_mouse_input(e)
  
   if e:up(2) then
      -- determine if the mouse has been clicked or dragged
      local dx = math.abs(e.x - self._mouse_down_x)
      local dy = math.abs(e.y - self._mouse_down_y)

      if not self._moved and dx < input_constants.mouse.dead_zone_size * 2 and dy < input_constants.mouse.dead_zone_size * 2 then
         self._moved = true
         self:_move_unit(e)
      end
   elseif e:down(2) then
      self._moved = false
      self._mouse_down_x = e.x
      self._mouse_down_y = e.y
   end
   return true
end

function UnitControlService:_move_unit(e)
   local selected_entity = _radiant.client.get_selected_entity()
   if selected_entity then
      local move_location
      local ray = _radiant.renderer.scene.cast_screen_ray(e.x, e.y)
      local screen_ray = _radiant.renderer.scene.get_screen_ray(e.x, e.y)

      if ray:is_valid() then
         move_location = ray:intersection_of(0)
      else 
         move_location = screen_ray.origin
      end

      _radiant.call('stonehearth:server_move_unit', selected_entity:get_id(), move_location)
      :done(function(r)
            --response:resolve(true)
         end)
      :always(function()
            --cleanup()
         end)
   end
end  

return UnitControlService
