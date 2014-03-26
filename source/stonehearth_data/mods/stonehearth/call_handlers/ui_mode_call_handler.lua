local UiModeCallHandler = class()

function UiModeCallHandler:ui_mode_changed(session, request, mode)
  radiant.events.trigger(radiant.events, 'stonehearth:ui_mode_changed', {
    mode = mode
  })
end

return UiModeCallHandler
