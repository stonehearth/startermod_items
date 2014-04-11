local UiModeCallHandler = class()

local ui_mode = 'normal'

function UiModeCallHandler:ui_mode_changed(session, request, mode)
   radiant.events.trigger_async(radiant.events, 'stonehearth:ui_mode_changed', {
   	mode = mode
   })
   ui_mode = mode
end

function UiModeCallHandler:get_ui_mode(session, request)

   return {
      -- these boxed regions will serialize into a uri handle
      mode = ui_mode
   }
end


return UiModeCallHandler
