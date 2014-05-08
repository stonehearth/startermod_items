local UiModeCallHandler = class()

local ui_mode = 'normal'

function UiModeCallHandler:set_ui_mode(session, request, mode)
   stonehearth.renderer:set_ui_mode(mode)
   return true
end

function UiModeCallHandler:set_building_vision_mode(session, request, mode)
   stonehearth.renderer:set_building_vision_mode(mode)
   return true
end

function UiModeCallHandler:get_ui_mode(session, request)
   return {
      mode = stonehearth.renderer:get_ui_mode()
   }
end

function UiModeCallHandler:get_building_vision_mode(session, request)
   return {
      mode = stonehearth.renderer:get_building_vision_mode()
   }
end

return UiModeCallHandler
