
local SetPosture = class()
SetPosture.name = 'set posture'
SetPosture.does = 'stonehearth:set_posture'
SetPosture.args = {
   posture = 'string'
}
SetPosture.version = 2
SetPosture.priority = 1

function SetPosture:start(ai, entity, args)
   radiant.entities.set_posture(entity, args.posture)
end

function SetPosture:stop(ai, entity, args)
   radiant.entities.unset_posture(entity, args.posture)
end

return SetPosture
