
local WaitForTimeOfDayEncounter = class()

function WaitForTimeOfDayEncounter:initialize()
   self._log = radiant.log.create_logger('game_master.encounters.wait_for_time_of_day')
end

function WaitForTimeOfDayEncounter:start(ctx, info)
   assert(info.time)

   local time = info.time
   self._sv.time = time
   self.__saved_variables:mark_changed()

   self._log:info('setting wait_for_time_of_day timer for %s', tostring(time))
   self._timer = stonehearth.calendar:set_alarm(time, function()
         self._timer = nil
         ctx.arc:trigger_next_encounter(ctx)
      end)
end

function WaitForTimeOfDayEncounter:stop()
   if self._timer then
      self._timer:destroy()
      self._timer = nil
   end
end

return WaitForTimeOfDayEncounter

