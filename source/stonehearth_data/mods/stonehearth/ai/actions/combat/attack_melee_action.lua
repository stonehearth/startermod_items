local Entity = _radiant.om.Entity

local AttackMelee = class()

AttackMelee.name = 'attack melee'
AttackMelee.does = 'stonehearth:combat:attack'
AttackMelee.args = {
   target = Entity
}
AttackMelee.version = 2
AttackMelee.priority = 1
AttackMelee.weight = 1

local ai = stonehearth.ai
return ai:create_compound_action(AttackMelee)
   :execute('stonehearth:combat:get_melee_range', {
      target = ai.ARGS.target,
   })
   :execute('stonehearth:combat:create_engage_callback', {
      target = ai.ARGS.target,
   })
   :execute('stonehearth:chase_entity', {
      target = ai.ARGS.target,
      stop_distance = ai.BACK(2).melee_range_ideal,
      grid_location_changed_cb = ai.PREV.callback,
   })
   :execute('stonehearth:bump_allies', {
      distance = 2,
   })
   :execute('stonehearth:combat:attack_melee_adjacent', {
      target = ai.ARGS.target,
   })
