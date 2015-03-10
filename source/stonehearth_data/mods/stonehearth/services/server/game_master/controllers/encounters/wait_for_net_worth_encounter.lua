
local WaitForNetWorthEncounter = class()

function WaitForNetWorthEncounter:activate()
   self._log = radiant.log.create_logger('game_master.encounters.wait_for_net_worth')
end

function WaitForNetWorthEncounter:start(ctx, info)
   assert(info)
   assert(info.threshold)

   self._sv.ctx = ctx
   self._sv.threshold = info.threshold
   self.__saved_variables:mark_changed()

   if radiant.util.get_config('game_master.encounters.wait_for_net_worth.always_trigger') then
      self._log:info('trigging next edge immediately')
      ctx.arc:trigger_next_encounter(ctx)         
      return
   end

   self._timer = stonehearth.calendar:set_interval('1h', function()
         local threshold = self._sv.threshold
         local total = self:_get_current_net_worth()
         self._log:info('checking net worth score.  %d >? %d', total, threshold)
         if total > threshold then
            self._log:info('triggering next edge')
            ctx.arc:trigger_next_encounter(ctx)
         end
      end)
end

function WaitForNetWorthEncounter:stop()
   if self._timer then
      self._timer:destroy()
      self._timer = nil
   end
end

function WaitForNetWorthEncounter:_get_current_net_worth()
   local ctx = self._sv.ctx
   local score_data = stonehearth.score:get_scores_for_player(ctx.player_id)
                              :get_score_data()
   if not score_data or not score_data.net_worth then
      self._log:info('no net worth score data yet.')
      return 0
   end
   return score_data.net_worth.total_score
end

-- debug commands sent by the ui

function WaitForNetWorthEncounter:get_progress_cmd(session, response)
   return {
      threshold = self._sv.threshold,
      current = self:_get_current_net_worth(),
   }
end

function WaitForNetWorthEncounter:trigger_now_cmd(session, response)
   local ctx = self._sv.ctx

   self._log:info('spawning encounter now as requested by ui')
   ctx.arc:trigger_next_encounter(ctx)
   return true
end

return WaitForNetWorthEncounter
