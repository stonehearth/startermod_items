
local UnsetPosture = class()
UnsetPosture.name = 'unset posture'
UnsetPosture.does = 'stonehearth:unset_posture'
UnsetPosture.args = {
   posture = 'string'
}
UnsetPosture.version = 2
UnsetPosture.priority = 1

function UnsetPosture:run(ai, entity, args)
   radiant.entities.unset_posture(entity, args.posture)
end

return UnsetPosture
