local EffectTrack = require 'modules.effects.effect_track'
local AnimationEffect = class(EffectTrack)

function AnimationEffect:__init(animation_path, start_time, info)  
   self[EffectTrack]:__init(info)

   self._start_time = start_time + self:_get_start_time()

   self._animation = radiant.resources.load_animation(animation_path)
   if not self._animation then
      log:warning('could not lookup animation resource %s', animation_path)
   end
   -- simulation effects don't honor the loop flag.  if you'd like
   -- to loop, simply start the effect and don't destroy it until you'd
   -- like it to stop.  You still need to set the loop flag to ensure
   -- the client loops it, though.
   local speed = info.speed or 1
   self._end_time = self._start_time + self._animation:get_duration() / speed
end

function AnimationEffect:update(e)
   return self._end_time == 0 or e.now <= self._end_time
end

return AnimationEffect
