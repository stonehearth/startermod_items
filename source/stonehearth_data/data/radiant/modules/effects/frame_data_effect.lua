local Effect = require 'radiant.modules.effects.effect'
local FrameDataEffect = class(Effect)

function FrameDataEffect:__init(start_time, handler, info, effect)
   self[Effect]:__init(info)

   self._effect = effect
   self._start_time = start_time
   self._segments = effect_data.segments
   self._segment_index = 0
   self._handler = handler
end

function FrameDataEffect:update(now)
   local segment
   if self._segment_index == 0 then
      self._segment_index = 1
      segment = self._segments[1]
      if self._handler then
         md:send_msg(self._handler, 'radiant.effect.frame_data.on_segment', 'start', segment)
      end
   else
      segment = self._segments[self._segment_index]
   end
  
   while segment and self._start_time + frame_count_to_time(segment.duration) <= now do
      if self._handler then
         md:send_msg(self._handler, 'radiant.effect.frame_data.on_segment', 'end', segment)
      end
      
      self._segment_index = self._segment_index + 1
      segment = self._segments[self._segment_index]
      if segment and self._handler then
         md:send_msg(self._handler, 'radiant.effect.frame_data.on_segment', 'start', segment)
      end
   end
   return segment ~= nil
end

return FrameDataEffect