local CalendarTimer = class()

function CalendarTimer:__init(expire_time, fn, repeating)
   self.active = true
   self.expire_time = expire_time
   self.repeating = repeating
   self.fn = fn
end

function CalendarTimer:destroy()
   self.active = false
end

function CalendarTimer:get_expire_time()
   return self.expire_time
end

function CalendarTimer:get_remaining_time()
   return self.expire_time - stonehearth.calendar:get_elapsed_game_time()
end

return CalendarTimer
