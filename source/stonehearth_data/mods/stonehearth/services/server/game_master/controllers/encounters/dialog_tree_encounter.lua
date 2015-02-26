local game_master_lib = require 'lib.game_master.game_master_lib'

local DialogTreeEncounter = class()

function DialogTreeEncounter:start(ctx, info)
   self._sv.ctx = ctx
   self._sv._info = info
   self._sv.bulletin_data = {}

   self._log = radiant.log.create_logger('game_master.encounters.dialog_tree')   

   assert(info.start_node)
   assert(info.nodes)
   game_master_lib.compile_bulletin_nodes(info.nodes, ctx)

   self._sv.dialog_tree = info.nodes
   self:_transition_to_node(info.start_node)
end

function DialogTreeEncounter:stop()
   self:_destroy_bulletin()
end

-- return the edge to transition to, which is simply the result of the encounter.
-- this will be one of the edges listed in the `out_edges` (refuse, fail, or success)
--
function DialogTreeEncounter:get_out_edge()
   return self._sv.resolved_out_edge
end

-- start of the state machine.  introduce the guy doing the shakedown
--
function DialogTreeEncounter:_transition_to_node(name)
   local ctx = self._sv.ctx

   local node = self._sv.dialog_tree[name]
   if not node then
      self._log:error('no dialog tree node named \"%s\"', name)
      ctx.arc:terminate(ctx)
      return
   end

   local bulletin = self._sv.bulletin
   if not bulletin then
      local player_id = ctx.player_id
      bulletin = stonehearth.bulletin_board:post_bulletin(player_id)
                                    :set_ui_view('StonehearthDialogTreeBulletinDialog')
                                    :set_callback_instance(self)
                                    :set_type('quest')
                                    :set_sticky(true)
                                    :set_keep_open(true)
                                    :set_close_on_handle(false)
      self._sv.bulletin = bulletin
   end
   bulletin:set_data(node.bulletin)
end

-- called by the ui when the intro's finished.  transition into the demand
-- phase
--
function DialogTreeEncounter:_on_dialog_tree_choice(session, response, choice)
   local ctx = self._sv.ctx
   local bulletin = self._sv.bulletin
   if not bulletin then
      self._log:error('no current bulletin in _on_dialog_tree_choice (choice:%s)', choice)
      ctx.arc:terminate(ctx)
      return false
   end

   local data = bulletin:get_data()
   local choice = data.choices[choice]
   if not choice then
      self._log:error('unexpected choice in _on_dialog_tree_choice (choice:%s)', choice)
      ctx.arc:terminate(ctx)
      return false
   end
   if choice.out_edge then
      self:_finish_encounter(choice.out_edge)
      return true
   end
   if not choice.next_node then
      self._log:error('missnig next_node for choice in _on_dialog_tree_choice (choice:%s)', choice)
      ctx.arc:terminate(ctx)
      return false
   end
   self:_transition_to_node(choice.next_node)
end

-- transition out of the encounter using the specified out_edge
--
function DialogTreeEncounter:_finish_encounter(out_edge)
   self:_destroy_bulletin()
   
   self._sv.resolved_out_edge = out_edge
   self.__saved_variables:mark_changed()

   local ctx = self._sv.ctx
   ctx.arc:trigger_next_encounter(ctx)
end

-- ends the current conversation
--
function DialogTreeEncounter:_destroy_bulletin()
   local bulletin = self._sv.bulletin
   if bulletin then
      stonehearth.bulletin_board:remove_bulletin(bulletin)
      self._sv.bulletin = nil
      self.__saved_variables:mark_changed()
   end
end

return DialogTreeEncounter

