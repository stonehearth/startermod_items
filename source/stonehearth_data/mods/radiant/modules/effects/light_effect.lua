local EffectTrack = require 'modules.effects.effect_track'
local LightEffect = radiant.class(EffectTrack)

function LightEffect:update(now)
   return true
end

return LightEffect
