local Effect = require 'modules.effects.effect'
local TriggerEffect = class(Effect)

function TriggerEffect:__init(start_time, handler, track, effect, entity, args)
   self[Effect]:__init(track)
   --May be redundant with above?
   self._info = track.info
   self._entity = entity
   self._args = args

   self._effect = effect
   local proposed_start = self:_get_start_time()
   self._trigger_time = start_time + proposed_start
   self._handler = handler
end

function TriggerEffect:update(e)
   if self._trigger_time and self._trigger_time <= e.now then
      if self._handler then
         self._handler(self._info, self._effect, self._entity)
      else
         if self._effect._trigger_cb then
            self._effect._trigger_cb(self._info, self._args)
         end
      end
      self._trigger_time = nil
   end
   return self._trigger_time ~= nil
end

return TriggerEffect