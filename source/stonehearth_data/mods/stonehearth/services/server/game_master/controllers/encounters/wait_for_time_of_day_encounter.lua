
local WaitForTimeOfDayEncounter = class()

function WaitForTimeOfDayEncounter:activate()
   self._log = radiant.log.create_logger('game_master.encounters.wait_for_time_of_day')
   if self._sv.timer then
      self._sv.timer:bind(function()
            self:_on_alarm_fired()
         end)
   end
end

function WaitForTimeOfDayEncounter:start(ctx, info)
   assert(info.time)

   local time = info.time
   self._sv.ctx = ctx
   self._sv.time = time

   self._log:info('setting wait_for_time_of_day timer for %s', tostring(time))
   self._sv.timer = stonehearth.calendar:set_alarm(time, function()
         self:_on_alarm_fired()
      end)
   self.__saved_variables:mark_changed()
end

function WaitForTimeOfDayEncounter:stop()
   if self._sv.timer then
      self._sv.timer:destroy()
      self._sv.timer = nil
      self.__saved_variables:mark_changed()
   end
end

function WaitForTimeOfDayEncounter:_on_alarm_fired()
   local ctx = self._sv.ctx

   self._sv.timer:destroy()
   self._sv.timer = nil
   self.__saved_variables:mark_changed()
   ctx.arc:trigger_next_encounter(ctx)
end

-- debug commands sent by the ui

function WaitForTimeOfDayEncounter:get_progress_cmd(session, response)
   local progress = {
      alarm_time = self._sv.time,
      current_time = stonehearth.calendar:format_time(),
   }
   if self._sv.timer then
      progress.time_left = stonehearth.calendar:format_remaining_time(self._sv.timer)
   end
   return progress;
end

function WaitForTimeOfDayEncounter:trigger_now_cmd(session, response)
   local ctx = self._sv.ctx

   self._log:info('triggering now as requested by ui')   
   if self._sv.timer then
      self._sv.timer:destroy()
      self._sv.timer = nil
      self.__saved_variables:mark_changed()
   end
   ctx.arc:trigger_next_encounter(ctx)
   return true
end

return WaitForTimeOfDayEncounter

