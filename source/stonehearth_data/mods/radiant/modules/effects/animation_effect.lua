local Effect = require 'modules.effects.effect'
local AnimationEffect = class(Effect)

function AnimationEffect:__init(animation_path, start_time, info)  
   self[Effect]:__init(info)

   self._start_time = start_time + self:_get_start_time()

   self._animation = radiant.resources.load_animation(animation_path)
   if not self._animation then
      log:warning('could not lookup animation resource %s', animation_path)
   end
   -- simulation effects don't honor the loop flag.  if you'd like
   -- to loop, simply start the effect and don't destroy it until you'd
   -- like it to stop.  You still need to set the loop flag to ensure
   -- the client loops it, though.
   self._end_time = self._start_time + self._animation:get_duration()
end

function AnimationEffect:update(e)
   local now = e.now
   return self._end_time == 0 or self._end_time >= now
end

return AnimationEffect
