local log = radiant.log.create_logger('combat')

BatteryContext = class()

-- placeholder
function BatteryContext:__init(attacker, damage)
   self.attacker = attacker
   self.damage = damage
end

return BatteryContext
