local EffectTrack = require 'modules.effects.effect_track'
local SoundEffect = class(EffectTrack)

function SoundEffect:__init(start_time, info)
   self[EffectTrack]:__init(info)
   self._info = info
   self._loop = false
   self._start_time = start_time + self:_get_start_time()
   
   if self._info.loop ~= nil then
      self._loop = self._info.loop
   end

   if info.end_time then
      self._end_time = info.end_time
   elseif info.duration then
      -- the server uses the duration to determine how long to keep the effect alive
      -- before automatically removing it from the entity.
      self._end_time = self._start_time + info.duration
   end
end

--Update returns true if we should continue, false if we should stop
--If we're looping, return true. Otherwise, return if we've past the end time
function SoundEffect:update(e)
   -- if there's an end time, definitely stop then. -- tony
   if self._end_time then
      return e.now <= self._end_time
   end
   return self._loop
end

return SoundEffect