local Effect = require 'modules.effects.effect'
local GenericEffect = class(Effect)

function GenericEffect:__init(start_time, handler, info, effect)
   self[Effect]:__init(info)

   self._end_time = start_time + self:_get_end_time()
   if not self._end_time then
      self._end_time = 0
   end
end

function GenericEffect:update(now)  
   return now < self._end_time
end

return GenericEffect
