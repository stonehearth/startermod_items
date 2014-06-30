
local SetPosture = class()
SetPosture.name = 'set posture'
SetPosture.does = 'stonehearth:set_posture'
SetPosture.args = {
   posture = 'string'
}
SetPosture.version = 2
SetPosture.priority = 1

function SetPosture:start(ai, entity, args)
   -- perform this in start, because UnsetPosture is called in stop,
   -- and they need to be a transacitonal pair
   radiant.entities.set_posture(entity, args.posture)
end

return SetPosture
