local EffectTrack = class()

function EffectTrack:__init(info)
   assert(info)
   self._info = info
end

function EffectTrack:_frame_count_to_time(duration)
   return math.floor(1000 * duration / 30)
end

function EffectTrack:_get_start_time()
   if self._info.start_time then
      return self._info.start_time
   end
   if self._info.start_frame then
      return self:_frame_count_to_time(self._info.start_frame)
   end
   return 0
end

function EffectTrack:_get_end_time()
   if self._info.end_time then
      return self._info.end_time
   end
   if self._info.end_frame then
      return self:_frame_count_to_time(self._info.end_frame)
   end
   if self._info.loop then
      return 0
   end
   if self._info.duration then
      return self:_get_start_time() + self._info.duration
   end
   if self._info.type == 'animation_effect' then
      local animation = _radiant.res.load_animation(self._info.animation)
      assert(animation)
      return animation and animation:get_duration() or 0
   end
   if self._info.type == 'attach_item_effect' then
      --Assume that if we don't have any other data, attaching is instantaneous
      if self._info.start_frame then
         return self:_frame_count_to_time(self._info.start_frame)
      else
         return 0
      end
   end
   assert(false)
   return 0
end

return EffectTrack
