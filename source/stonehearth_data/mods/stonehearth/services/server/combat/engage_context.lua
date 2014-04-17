local log = radiant.log.create_logger('combat')

EngageContext = class()

function EngageContext:__init(attacker)
   self.attacker = attacker
end

return EngageContext
