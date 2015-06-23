local EffectTrack = require 'modules.effects.effect_track'
local ActivityOverlayEffect = radiant.class(EffectTrack)

function ActivityOverlayEffect:__init(info)
   EffectTrack.__init(self, info)
end

function ActivityOverlayEffect:update(now)
   return true
end

return ActivityOverlayEffect
