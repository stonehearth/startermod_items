local Effect = require 'modules.effects.effect'
local MusicEffect = class(Effect)

function MusicEffect:__init(start_time, handler, info, effect)
   self[Effect]:__init(info)
   self._info = info
   self._loop = true
   self._end_time = 0
   
   --If we're a music effect, we loop by default. If we're a sound effect, we do not loop by default
   if self._info.type == 'sound_effect' then
      self._loop = false;
   end

   --Now check if the user speified a loop variable. If so, assign it
   if self._info.loop ~= nil then
      self._loop = self._info.loop
   end

   --If we loop, duration doesn't matter. If we don't loop, make sure we have duration
   --TODO: get duration by looking at the actual file. Until then, duration is a required part of the JSON   
   if not self._loop then
      assert(self._info.duration)
      self._end_time = start_time + self._info.duration
   end
end

--Update returns true if we should continue, false if we should stop
--If we're looping, return true. Otherwise, return if we've past the end time
function MusicEffect:update(now)
   return self._loop or self._end_time >= now
end

return MusicEffect