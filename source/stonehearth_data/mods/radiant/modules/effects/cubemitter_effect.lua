local EffectTrack = require 'modules.effects.effect_track'
local CubemitterEffect = class(EffectTrack)

function CubemitterEffect:__init(info)  
   self[EffectTrack]:__init(info)
end

function CubemitterEffect:update(now)
   return true
end

return CubemitterEffect
