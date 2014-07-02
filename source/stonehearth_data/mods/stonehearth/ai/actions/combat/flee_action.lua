local Entity = _radiant.om.Entity

local Flee = class()
Flee.name = 'combat flee'
Flee.status_text = 'fleeing'
Flee.does = 'stonehearth:combat:panic'
Flee.args = {
   threat = Entity,
}
Flee.version = 2
Flee.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(Flee)
         :execute('stonehearth:run_away_from_entity', {
            threat = ai.ARGS.threat,
            distance = 32,
         })
         :execute('stonehearth:turn_to_face_entity', {
            entity = ai.ARGS.threat,
         })
