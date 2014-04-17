local log = radiant.log.create_logger('combat')

AssaultContext = class()

function AssaultContext:__init(attack_method, attacker, impact_time)
   self.attack_method = 'melee'
   self.attacker = attacker
   self.impact_time = impact_time

   self.target_defending = false
end

return AssaultContext
