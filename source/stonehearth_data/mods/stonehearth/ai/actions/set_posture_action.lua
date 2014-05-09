
local SetPosture = class()
SetPosture.name = 'set posture'
SetPosture.does = 'stonehearth:set_posture'
SetPosture.args = {
   posture = 'string'
}
SetPosture.version = 2
SetPosture.priority = 1

function SetPosture:run(ai, entity, args)
   radiant.entities.set_posture(entity, args.posture)
end

return SetPosture
