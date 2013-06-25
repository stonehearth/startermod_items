local Effect = require 'radiant.modules.effects.effect'
local MusicEffect = class(Effect)

function MusicEffect:__init(start_time, handler, info, effect)
   self[Effect]:__init(info)
   self._end_time = start_time + self:_get_end_time()
   if not self._end_time then
      self._end_time = 0
   end
end

--If we're looping, don't do anything.
function MusicEffect:update(now)
   return self._end_time == 0 or self._end_time >= now
end

return MusicEffect