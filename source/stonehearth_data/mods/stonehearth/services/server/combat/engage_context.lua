local log = radiant.log.create_logger('combat')

EngageContext = class()

function EngageContext:__init(attacker, target)
   self.attacker = attacker
   self.target = target
end

return EngageContext
