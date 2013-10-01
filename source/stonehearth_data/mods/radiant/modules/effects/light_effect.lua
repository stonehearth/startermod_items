local Effect = require 'modules.effects.effect'
local LightEffect = class(Effect)

function LightEffect:__init(info)  
   self[Effect]:__init(info)
end

function LightEffect:update(now)
   return true
end

return LightEffect
