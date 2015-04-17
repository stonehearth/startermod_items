local StopPanicking = class()
StopPanicking.name = 'stop panicking'
StopPanicking.does = 'stonehearth:combat:stop_panicking'
StopPanicking.args = {}
StopPanicking.version = 2
StopPanicking.priority = 1
StopPanicking.realtime = true

function StopPanicking:run(ai, entity, args)
   stonehearth.combat:set_panicking_from(entity, nil)
end

function StopPanicking:stop(ai, entity, args)
end

return StopPanicking
