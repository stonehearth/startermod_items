local Effect = require 'modules.effects.effect'
local ActivityOverlayEffect = class(Effect)

function ActivityOverlayEffect:__init(info)  
   self[Effect]:__init(info)
end

function ActivityOverlayEffect:update(now)
   return true
end

return ActivityOverlayEffect
