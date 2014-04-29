local log = radiant.log.create_logger('combat')

BatteryContext = class()

-- placeholder
function BatteryContext:__init(damage)
   self.damage = damage
end

return BatteryContext
