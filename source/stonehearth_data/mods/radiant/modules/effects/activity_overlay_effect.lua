local EffectTrack = require 'modules.effects.effect_track'
local ActivityOverlayEffect = class(EffectTrack)

function ActivityOverlayEffect:__init(info)  
   self[EffectTrack]:__init(info)
end

function ActivityOverlayEffect:update(now)
   return true
end

return ActivityOverlayEffect
