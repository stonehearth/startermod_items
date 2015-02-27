
local WaitForTimeOfDayEncounter = class()

function WaitForTimeOfDayEncounter:initialize()
   self._log = radiant.log.create_logger('game_master.encounters.wait_for_time_of_day')
end

function WaitForTimeOfDayEncounter:start(ctx, info)
   assert(info.time)

   local time = info.time
   self._sv.ctx = ctx
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


-- debug commands sent by the ui

function WaitForTimeOfDayEncounter:get_progress_cmd(session, response)
   local progress = {
      alarm_time = self._sv.time,
      current_time = stonehearth.calendar:format_time(),
   }
   if self._timer then
      progress.time_left = stonehearth.calendar:format_remaining_time(self._timer)
   end
   return progress;
end

function WaitForTimeOfDayEncounter:trigger_now_cmd(session, response)
   local ctx = self._sv.ctx

   self._log:info('triggering now as requested by ui')   
   if self._timer then
      self._timer:destroy()
      self._timer = nil
   end
   ctx.arc:trigger_next_encounter(ctx)
   return true
end

return WaitForTimeOfDayEncounter

