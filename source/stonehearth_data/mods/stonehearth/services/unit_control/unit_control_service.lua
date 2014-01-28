local Point3 = _radiant.csg.Point3

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
   if e:down(2) then
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

--[[
  if drag_key_down and not self._dragging then
    local r = _radiant.renderer.scene.cast_screen_ray(e.x, e.y)
    local screen_ray = _radiant.renderer.scene.get_screen_ray(e.x, e.y)
    self._dragging = true
    self._drag_origin = screen_ray.origin;

    if r.is_valid then
      self._drag_start = r.point
    else
      local d = -self._drag_origin.y / screen_ray.direction.y
      screen_ray.direction:scale(d)
      self._drag_start = screen_ray.origin + screen_ray.direction
    end

    local root = _radiant.client.get_entity(1)
    local terrain_comp = root:get_component('terrain')
    local bounds = terrain_comp:get_bounds():to_float()
    if not bounds:contains(self._drag_start) then
      self._dragging = false
    end
  elseif not drag_key_down and self._dragging then
    self._dragging = false
  end

  if not self._dragging then
    return
  end

  ]]  
end

UnitControlService:__init()
return UnitControlService