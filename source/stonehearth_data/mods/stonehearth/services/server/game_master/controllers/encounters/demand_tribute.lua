
local DemandTribute = class()

function DemandTribute:start(ctx, info)
   self._sv.ctx = ctx
   self._sv.info = info
   self._sv.bulletin_data = {}

   assert(info.script)
   assert(info.out_edges)   
   assert(info.introduction)
   assert(info.introduction.bulletin)
   assert(info.shakedown)
   assert(info.shakedown.bulletin)
   assert(info.script)

   local script = radiant.create_controller(info.script)
   self._sv.script = script

   assert(script)
   assert(script.get_tribute_demand)

   self._sv.demand = self._sv.script:get_tribute_demand(self._sv.ctx)
   self:_show_introduction()
end

-- return the edge to transition to, which is simply the result of the encounter.
-- this will be one of the edges listed in the `out_edges` (refuse, fail, or success)
--
function DemandTribute:get_out_edge()
   return self._sv.resolved_out_edge
end

-- start of the state machine.  introduce the guy doing the shakedown
--
function DemandTribute:_show_introduction()
   local intro = self._sv.info.introduction.bulletin

   intro.next_callback = '_on_introduction_finished'
   self:_update_bulletin(intro)
end

-- called by the ui when the intro's finished.  transition into the demand
-- phase
--
function DemandTribute:_on_introduction_finished()
   self:_show_demand()
end

-- the demand phase.  state the demands!
--
function DemandTribute:_show_demand()
   local bulletin_data = self._sv.info.shakedown.bulletin

   bulletin_data.demands = {
   	items = self._sv.demand
	}
   bulletin_data.accepted_callback = '_on_shakedown_accepted'
   bulletin_data.declined_callback = '_on_shakedown_declined'
   self:_update_bulletin(bulletin_data, { view = 'StonehearthDemandTributeBulletinDialog' })
end

-- called by the ui if the player accepts the terms of the shakedown.
--
function DemandTribute:_on_shakedown_accepted()
   self:_destroy_bulletin()
   -- xxx: for testing, loop back to the intro.
   self:_show_introduction()
end

-- called by the ui if the player declines the terms of the shakdown.
-- show the antagonist's displeasure
--
function DemandTribute:_on_shakedown_declined()
   self:_show_refused_threat()
end

-- show the antagonist's displeasure.  called when the shakedown is rejected
--
function DemandTribute:_show_refused_threat()
   assert(self._sv.bulletin)

   local bulletin_data = self._sv.info.shakedown_refused.bulletin

   bulletin_data.ok_callback = '_on_refused_threat_ok'
   self:_update_bulletin(bulletin_data)
end

-- after the user clicks the ok button, transition to the next encounter
-- in the arc (after setting the 'refuse' out_edge)
--
function DemandTribute:_on_refused_threat_ok()
   radiant.log.write('', 0, 'got ok')
   self:_finish_encounter('refuse')
end

-- transition out of the encounter using the edge in the data file named after
-- `result`.  `result` must be one of 'success', 'fail', or 'reject'
--
function DemandTribute:_finish_encounter(result)
   self:_destroy_bulletin()

   assert(out_edge)
   local out_edge = self._sv.info.out_edges[result]
   self._sv.resolved_out_edge = out_edge
   self.__saved_variables:mark_changed()

   local ctx = self._sv.ctx
   ctx.arc:trigger_next_encounter(ctx)
end

-- update the conversation tree with the new bulletin data.
--
function DemandTribute:_update_bulletin(new_bulletin_data, opt)
   local ctx = self._sv.ctx
   local player_id = ctx.player_id
   local opt_view = (opt and opt.view) or 'StonehearthGenericBulletinDialog'

   local bulletin = self._sv.bulletin
   if not bulletin then
      bulletin = stonehearth.bulletin_board:post_bulletin(ctx.player_id)
                                    :set_callback_instance(self)
                                    :set_type('quest')
                                    :set_sticky(true)
                                    :set_keep_open(true)
                                    :set_close_on_handle(false)
      self._sv.bulletin = bulletin
   end

   self._sv.bulletin_data = radiant.copy_table(new_bulletin_data)
   self.__saved_variables:mark_changed()

   bulletin:set_data(self._sv.bulletin_data)
   		  :set_ui_view(opt_view)

end

-- ends the current conversation
--
function DemandTribute:_destroy_bulletin()
   local bulletin = self._sv.bulletin
   if bulletin then
      stonehearth.bulletin_board:remove_bulletin(bulletin)
      self._sv.bulletin = nil
      self.__saved_variables:mark_changed()
   end
end

return DemandTribute

