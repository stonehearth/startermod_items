local Entity = _radiant.om.Entity
local log = radiant.log.create_logger('combat')

local CombatIdleReady = class()

CombatIdleReady.name = 'combat idle ready'
CombatIdleReady.does = 'stonehearth:combat:idle'
CombatIdleReady.args = {
   enemy = Entity
}
CombatIdleReady.version = 2
CombatIdleReady.priority = 1
CombatIdleReady.weight = 1

function CombatIdleReady:run(ai, entity, args)
   if not args.enemy:is_valid() then
      ai:abort('enemy has been destroyed')
      return
   end

   ai:execute('stonehearth:turn_to_face_entity', { entity = args.enemy })
   
   -- TODO: get the idle animation appropriate to the weapon
   ai:execute('stonehearth:run_effect', {
      effect = 'combat_1h_idle'
   })
end

return CombatIdleReady
