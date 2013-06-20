local Effect = require 'radiant.modules.effects.effect'
local AnimationEffect = class(Effect)

function AnimationEffect:__init(animation_path, start_time, info)  
   self[Effect]:__init(info)

   self._loop = info.loop
   self._start_time = start_time + self:_get_start_time()

   self._animation = radiant.resources.load_animation(animation_path)
   if not self._animation then
      log:warning('could not lookup animation resource %s', animation_path)
   end
   radiant.check.is_a(self._animation, Animation)

   self._end_time = 0
   if not self._loop then 
      self._end_time = self._start_time + self._animation:get_duration()
   end
end

function AnimationEffect:update(now)
   return self._end_time == 0 or self._end_time >= now
end

return AnimationEffect
