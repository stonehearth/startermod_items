local MeleeAttack = class()

MeleeAttack.name = 'melee attack!'
MeleeAttack.does = 'stonehearth.attack.melee'
MeleeAttack.priority = 0

function MeleeAttack:run(ai, entity, target)
   assert(target)
   --ai:execute('stonehearth.run_effect', 'melee_attack')
   radiant.log.info('melee attack!')
end

return MeleeAttack