local Effect = require 'radiant.modules.effects.effect'
local TriggerEffect = class(Effect)

function TriggerEffect:__init(start_time, handler, info, effect)
   self[Effect]:__init(info)

   self._effect = effect
   self._trigger_time = start_time + get_start_time(effect_data)
   self._handler = handler
end

function TriggerEffect:update(now)
   if self._trigger_time and self._trigger_time <= now then
      md:send_msg(self._handler, "radiant.animation.on_trigger", self._info, self._effect)
      self._trigger_time = nil
   end
   return self._trigger_time ~= nil
end

return TriggerEffect