
local UnsetPosture = class()
UnsetPosture.name = 'unset posture'
UnsetPosture.does = 'stonehearth:unset_posture'
UnsetPosture.args = {
   posture = 'string'
}
UnsetPosture.version = 2
UnsetPosture.priority = 1

-- Always unset the posture by executing this in stop, because we're almost always using this in a
-- compound action with a SetPosture / UnsetPosture pair that we want to be transactional.
-- If the compound action was pre-empted or failed, we want to restore to the previous posture
function UnsetPosture:stop(ai, entity, args)
   radiant.entities.unset_posture(entity, args.posture)
end

return UnsetPosture
