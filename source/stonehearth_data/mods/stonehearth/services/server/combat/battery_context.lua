local log = radiant.log.create_logger('combat')

BatteryContext = class()

-- placeholder
function BatteryContext:__init(attacker, target, damage)
   self.attacker = attacker
   self.target = target
   self.damage = damage
end

return BatteryContext
