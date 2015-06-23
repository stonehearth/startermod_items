local EffectTrack = require 'modules.effects.effect_track'
local TriggerEffect = radiant.class(EffectTrack)

function TriggerEffect:__init(start_time, handler, track, effect, entity, args)
   EffectTrack.__init(self, track)
   
   self._entity = entity
   self._args = args
   self._track_info = track.info

   self._effect = effect
   local proposed_start = self:_get_start_time()
   self._trigger_time = start_time + proposed_start
   self._handler = handler
end

function TriggerEffect:update(e)
   if self._trigger_time and self._trigger_time <= e.now then
      if self._handler then
         self._handler(self._track_info, self._effect, self._entity)
      else
         if self._effect._trigger_cb then
            self._effect._trigger_cb(self._track_info, self._args)
         end
      end
      self._trigger_time = nil
   end
   return self._trigger_time ~= nil
end

return TriggerEffect