local Effect = require 'radiant.modules.effects.effect'
local MusicEffect = class(Effect)

function MusicEffect:__init(start_time, handler, info, effect)
   self[Effect]:__init(info)
   self._info = info
   --Note: if were looping, self._end_time will be meaningless, but try setting it anyway
   self._end_time = start_time + self:_get_end_time()
end

--Update returns true if we should continue, false if we should stop
--If we're looping, return true. Otherwise, return if we've past the end time
function MusicEffect:update(now)
   return self._info.loop or self._end_time >= now
end

return MusicEffect