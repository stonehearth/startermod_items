local Entity = _radiant.om.Entity
local log = radiant.log.create_logger('combat')

local CombatShuffle = class()

CombatShuffle.name = 'combat shuffle'
CombatShuffle.does = 'stonehearth:combat:idle'
CombatShuffle.args = {
   facing_entity = Entity
}
CombatShuffle.version = 2
CombatShuffle.priority = 1
CombatShuffle.weight = 1

local ai = stonehearth.ai
return ai:create_compound_action(CombatShuffle)
         :execute('stonehearth:wander', {
            radius = 1,
            radius_min = 1,
         })
         :execute('stonehearth:turn_to_face_entity', {
            entity = ai.ARGS.facing_entity
         })
