local EffectTrack = require 'modules.effects.effect_track'
local GenericEffect = class(EffectTrack)

function GenericEffect:__init(start_time, handler, info, effect)
   self[EffectTrack]:__init(info)

   self._loops = info.loop;
   if not self._loops then
      self._end_time = start_time + self:_get_end_time()
   end
end

function GenericEffect:update(e)  
   return not self._loops and e.now < self._end_time
end

return GenericEffect
