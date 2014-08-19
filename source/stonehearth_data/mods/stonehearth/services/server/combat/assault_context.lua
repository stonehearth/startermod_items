local log = radiant.log.create_logger('combat')

AssaultContext = class()

function AssaultContext:__init(attack_method, attacker, target, impact_time)
   self.attack_method = attack_method
   self.attacker = attacker
   self.target = target
   self.impact_time = impact_time

   self.assault_active = true -- signals defender if assault is still in progress
   self.target_defending = false -- signals attacker if defender is still defending
end

return AssaultContext
