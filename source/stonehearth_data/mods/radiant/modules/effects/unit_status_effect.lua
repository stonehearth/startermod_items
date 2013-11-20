local Effect = require 'modules.effects.effect'
local UnitStatusEffect = class(Effect)

function UnitStatusEffect:__init(info)  
   self[Effect]:__init(info)
end

function UnitStatusEffect:update(now)
   return true
end

return UnitStatusEffect
