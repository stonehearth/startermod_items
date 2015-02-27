
local WaitEncounter = class()

function WaitEncounter:initialize()
   self._log = radiant.log.create_logger('game_master.encounters.wait')
end

function WaitEncounter:start(ctx, info)
   assert(info.duration)

   local timeout = info.duration
   local override = radiant.util.get_config('game_master.encounters.wait.duration')
   if override ~= nil then
      timeout = override
   end

   self._log:info('setting wait timer for %s', tostring(timeout))
   self._timer = stonehearth.calendar:set_timer(timeout, function()
         self._timer = nil
         ctx.arc:trigger_next_encounter(ctx)
      end)
end

function WaitEncounter:stop()
   if self._timer then
      self._timer:destroy()
      self._timer = nil
   end
end

-- debug commands sent by the ui

function WaitEncounter:get_progress_cmd(session, response)
   local progress = {}
   if self._timer then
      progress.time_left = stonehearth.calendar:format_remaining_time(self._timer)
   end
   return progress;
end

function WaitEncounter:trigger_now_cmd(session, response)
   local ctx = self._sv.ctx

   self._log:info('triggering now as requested by ui')   
   if self._timer then
      self._timer:destroy()
      self._timer = nil
   end
   ctx.arc:trigger_next_encounter(ctx)
   return true
end

return WaitEncounter

