local EffectTrack = require 'modules.effects.effect_track'
local SoundEffect = class(EffectTrack)

function SoundEffect:__init(start_time, info)
   self[EffectTrack]:__init(info)
   self._info = info
   self._loop = false
   self._start_time = start_time + self:_get_start_time()
   self._end_time = 0
   self._duration = self._info.duration or 0
   
   if self._info.loop ~= nil then
      self._loop = self._info.loop
   end

   -- sounds always play out now (instead of clipping on termination, e.g. at 2x speed)
   -- the duration is optional and is used to indicate conceptually when the track is finished (e.g. for callbacks)
   if not self._loop then
      self._end_time = self._start_time + self._duration
   end
end

--Update returns true if we should continue, false if we should stop
--If we're looping, return true. Otherwise, return if we've past the end time
function SoundEffect:update(e)
   return self._loop or self._end_time > e.now
end

return SoundEffect