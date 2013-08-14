local Effect = require 'radiant.modules.effects.effect'
local TriggerEffect = class(Effect)

function TriggerEffect:__init(start_time, handler, info, effect, entity)
   self[Effect]:__init(info)
   --May be redundant with above?
   self._info = info
   self._entity = entity

   self._effect = effect
   local proposed_start = self:_get_start_time()
   self._trigger_time = start_time + proposed_start
   self._handler = handler
end

function TriggerEffect:update(now)
   if self._trigger_time and self._trigger_time <= now then
      radiant.events.broadcast_msg('radiant.animation.on_trigger', self._info, self._effect, self._entity)
      self._trigger_time = nil
   end
   return self._trigger_time ~= nil
end

return TriggerEffect