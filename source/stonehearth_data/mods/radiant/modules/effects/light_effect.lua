local EffectTrack = require 'modules.effects.effect_track'
local LightEffect = class(EffectTrack)

function LightEffect:__init(info)  
   self[EffectTrack]:__init(info)
end

function LightEffect:update(now)
   return true
end

return LightEffect
