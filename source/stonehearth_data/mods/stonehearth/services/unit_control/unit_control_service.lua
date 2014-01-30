local Point3 = _radiant.csg.Point3
local input_constants = require('constants').input

local UnitControlService = class()

function UnitControlService:__init()
   self._input_capture = _radiant.client.capture_input()
   self._input_capture:on_input(function(e)
      self:_on_input(e)
            -- Don't consume the event, since the UI might want to do something, too.
      return false
    end)
end


function UnitControlService:_on_input(e) 
    if e.type == _radiant.client.Input.MOUSE then
      self:_on_mouse_input(e.mouse)
    elseif e.type == _radiant.client.Input.KEYBOARD then
      --self:_on_keyboard_input(e.keyboard)
    end
end

function UnitControlService:_on_mouse_input(e)
  
   if e:up(2) then
      -- determine if the mouse has been clicked or dragged
      local dx = math.abs(e.x - self._mouse_down_x)
      local dy = math.abs(e.y - self._mouse_down_y)

      if dx < input_constants.mouse.dead_zone_size and dy < input_constants.mouse.dead_zone_size then
        self:_move_unit(e)
      end

   elseif e:down(2) then
      self._mouse_down_x = e.x
      self._mouse_down_y = e.y
   end
end

function UnitControlService:_move_unit(e)
   local selected_entity = _radiant.client.get_selected_entity()
   if selected_entity then
      local move_location
      local ray = _radiant.renderer.scene.cast_screen_ray(e.x, e.y)
      local screen_ray = _radiant.renderer.scene.get_screen_ray(e.x, e.y)

      if ray.is_valid then
         move_location = ray.point
      else 
         move_location = screen_ray.origin
      end

      _radiant.call('stonehearth:move_unit', selected_entity:get_id(), move_location)
      :done(function(r)
            --response:resolve(true)
         end)
      :always(function()
            --cleanup()
         end)
   end
end  

UnitControlService:__init()
return UnitControlService