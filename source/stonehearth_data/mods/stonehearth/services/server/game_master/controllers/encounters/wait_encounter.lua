local log = radiant.log.create_logger('game_master_encounter')

local WaitEncounter = class()

function WaitEncounter:activate()
   self._log = radiant.log.create_logger('game_master.encounters.wait')
   if self._sv.timer then

      self._sv.timer:bind(function()
         self:_timer_callback()
      end)
   end
end

function WaitEncounter:start(ctx, info)
   assert(info.duration)

   local timeout = info.duration
   local override = radiant.util.get_config('game_master.encounters.wait.duration')
   if override ~= nil then
      timeout = override
   end

   self._log:info('setting wait timer for %s', tostring(timeout))
   self._sv.ctx = ctx
   self._sv.timer = stonehearth.calendar:set_timer("WaitEncounter wait timer", timeout, function()
         self:_timer_callback()
      end)

   log:spam('Wait Encounter: %s will expire at %s which is in %s', ctx.encounter_name, self._sv.timer:get_expire_time(), stonehearth.calendar:format_remaining_time(self._sv.timer))
   log:spam('It is currently %s', stonehearth.calendar:format_time())
end

function WaitEncounter:_timer_callback()
   if self._sv.timer then
      self._sv.timer:destroy()
      self._sv.timer = nil
   end

   log:spam('Wait Encounter: %s is now firing at %s', self._sv.ctx.encounter_name, stonehearth.calendar:format_time())

   self.__saved_variables:mark_changed()
   self._sv.ctx.arc:trigger_next_encounter(self._sv.ctx)
end

function WaitEncounter:stop()
   if self._sv.timer then
      self._sv.timer:destroy()
      self._sv.timer = nil
      self.__saved_variables:mark_changed()
   end
end

-- debug commands sent by the ui

function WaitEncounter:get_progress_cmd(session, response)
   local progress = {}
   if self._sv.timer then
      progress.time_left = stonehearth.calendar:format_remaining_time(self._sv.timer)
   end
   return progress;
end

function WaitEncounter:trigger_now_cmd(session, response)
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

return WaitEncounter

