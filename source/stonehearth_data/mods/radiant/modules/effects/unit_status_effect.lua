local EffectTrack = require 'modules.effects.effect_track'
local UnitStatusEffect = class(EffectTrack)

function UnitStatusEffect:__init(info, start_time)
   self[EffectTrack]:__init(info)

   if info.duration then
      self._end_time = start_time + self:_get_end_time()
   end
end


function UnitStatusEffect:update(e)  
   return not self._end_time or e.now < self._end_time
end

return UnitStatusEffect
