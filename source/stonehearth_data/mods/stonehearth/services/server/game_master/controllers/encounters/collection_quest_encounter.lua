local I18N = require 'lib.i18n.i18n'

local Entity = _radiant.om.Entity

local CollectionQuest = class()

function CollectionQuest:start(ctx, info)
   self._sv.ctx = ctx
   self._sv._info = info
   self._sv.bulletin_data = {}

   assert(info.script)
   assert(info.out_edges)   
   assert(info.duration)

   assert(info.bulletins)
   local bulletins = info.bulletins
   assert(bulletins.introduction)
   assert(bulletins.shakedown)
   assert(bulletins.collection_progress)
   assert(bulletins.collection_due)
   assert(bulletins.collection_failed)
   assert(bulletins.shakedown_refused)

   local i18n = I18N()
   for _, bulletin in pairs(bulletins) do
      for key, text in pairs(bulletin) do
         bulletin[key] = i18n:format_string(text, ctx)
      end
   end


   local script = radiant.create_controller(info.script, ctx)
   self._sv.script = script

   assert(script)
   assert(script.get_tribute_demand)

   if info.source_entity then
      local entity = ctx[info.source_entity]
      if radiant.util.is_a(entity, Entity) and entity:is_valid() then
         self._sv.source_entity = entity
         self:_start_source_listener()
      end
   end

   self._sv.demand = self._sv.script:get_tribute_demand()
   self:_show_introduction()
   self:_cache_player_tracking_data()
end

function CollectionQuest:stop()
   self:_stop_tracking_items()
   self:_stop_collection_timer()
   self:_stop_source_listener()
   self:_destroy_bulletin()
end

-- return the edge to transition to, which is simply the result of the encounter.
-- this will be one of the edges listed in the `out_edges` (refuse, fail, or success)
--
function CollectionQuest:get_out_edge()
   return self._sv.resolved_out_edge
end

-- start of the state machine.  introduce the guy doing the shakedown
--
function CollectionQuest:_show_introduction()
   local intro = self._sv._info.bulletins.introduction

   intro.next_callback = '_on_introduction_finished'
   self:_update_bulletin(intro)
end

-- called by the ui when the intro's finished.  transition into the demand
-- phase
--
function CollectionQuest:_on_introduction_finished()
   self:_show_demand()
end

-- the demand phase.  state the demands!
--
function CollectionQuest:_show_demand()
   local bulletin_data = self._sv._info.bulletins.shakedown
   bulletin_data.demands = {
      items = self._sv.demand
   }
   bulletin_data.accepted_callback = '_on_shakedown_accepted'
   bulletin_data.declined_callback = '_on_shakedown_declined'
   self:_update_bulletin(bulletin_data, { view = 'StonehearthCollectionQuestBulletinDialog' })
end

-- called by the ui if the player accepts the terms of the shakedown.
--
function CollectionQuest:_on_shakedown_accepted()
   self:_destroy_bulletin()

   -- update the bulletin to start tracking the collection progress
   local bulletin_data = self._sv._info.bulletins.collection_progress
   local items = self._sv.demand
   bulletin_data.demands = {
      items = self._sv.demand
   }
   self:_update_bulletin(bulletin_data, { view = 'StonehearthCollectionQuestBulletinDialog' })
   self:_start_tracking_items()

   -- start a timer for when to check up on the quest
   self:_start_collection_timer()
end

-- called by the ui if the player declines the terms of the shakdown.
-- show the antagonist's displeasure
--
function CollectionQuest:_on_shakedown_declined()
   self:_show_refused_threat()
end

-- show the antagonist's displeasure.  called when the shakedown is rejected
--
function CollectionQuest:_show_refused_threat()
   assert(self._sv.bulletin)

   local bulletin_data = self._sv._info.bulletins.shakedown_refused

   bulletin_data.ok_callback = '_on_refused_threat_ok'
   self:_update_bulletin(bulletin_data)
end

-- after the user clicks the ok button, transition to the next encounter
-- in the arc (after setting the 'refuse' out_edge)
--
function CollectionQuest:_on_refused_threat_ok()
   self:_finish_encounter('refuse')
end

-- transition out of the encounter using the edge in the data file named after
-- `result`.  `result` must be one of 'success', 'fail', or 'reject'
--
function CollectionQuest:_finish_encounter(result)
   assert(result)
   self:_destroy_bulletin()

   local out_edge = self._sv._info.out_edges[result]
   assert(out_edge)
   
   self._sv.resolved_out_edge = out_edge
   self.__saved_variables:mark_changed()

   local ctx = self._sv.ctx
   ctx.arc:trigger_next_encounter(ctx)
end

-- update the conversation tree with the new bulletin data.
--
function CollectionQuest:_update_bulletin(new_bulletin_data, opt)
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

   self._sv.bulletin_data = radiant.shallow_copy(new_bulletin_data)
   self.__saved_variables:mark_changed()

   bulletin:set_data(self._sv.bulletin_data)
           :set_ui_view(opt_view)

end

-- ends the current conversation
--
function CollectionQuest:_destroy_bulletin()
   local bulletin = self._sv.bulletin
   if bulletin then
      stonehearth.bulletin_board:remove_bulletin(bulletin)
      self._sv.bulletin = nil
      self.__saved_variables:mark_changed()
   end
end

function CollectionQuest:_start_tracking_items()
   self._progress_trace = self._player_inventory_tracker:trace('quest progress')
                              :on_changed(function()
                                    self:_update_progress()
                                 end)
                              :push_object_state()
end

function CollectionQuest:_stop_tracking_items(items)
   if self._progress_trace then
      self._progress_trace:destroy()
      self._progress_trace = nil
   end
end

function CollectionQuest:_update_progress(items)
   assert(self._player_inventory_tracking_data)

   local bulletin = self._sv.bulletin
   if not bulletin then
      return
   end
   local tracking_data = self._player_inventory_tracking_data
   local items = self._sv.demand

   self._sv.have_enough = true
   for _, item in pairs(items) do
      item.progress = 0
      local info = tracking_data[item.uri]
      if info then
         item.progress = math.min(info.count, item.count)
         if item.progress < item.count then
            self._sv.have_enough = false
         end
      end
   end
   self.__saved_variables:mark_changed()
   
   bulletin:mark_data_changed()
end

function CollectionQuest:_start_collection_timer()
   assert(not self._collection_timer)

   local duration = self._sv._info.duration
   local override = radiant.util.get_config('game_master.encounters.collection_quest.duration')
   if override ~= nil then
      duration = override
   end

   self._collection_timer = stonehearth.calendar:set_timer(duration, function()
         self._collection_timer = nil
         self:_on_collection_timer_expired()
      end)
end

function CollectionQuest:_stop_collection_timer()
   if self._collection_timer then
      self._collection_timer:destroy()
      self._collection_timer = nil
   end
end

function CollectionQuest:_on_collection_timer_expired()
   self:_stop_tracking_items()

   local bulletin_data = self._sv._info.bulletins.collection_due
   bulletin_data.demands = {
      items = self._sv.demand,      
   }
   bulletin_data.have_enough = self._sv.have_enough

   bulletin_data.collection_pay_callback = '_on_collection_paid'
   bulletin_data.collection_cancel_callback = '_on_collection_cancelled'
   self:_update_bulletin(bulletin_data, { view = 'StonehearthCollectionQuestBulletinDialog' })   
end

function CollectionQuest:_on_collection_paid()
   -- take the items!
   local tracking_data = self._player_inventory_tracking_data

   for uri, info in pairs(self._sv.demand) do
      local entities = tracking_data[uri].items
      for i=1, info.count do
         local id, entity = next(entities)
         if entity then
            radiant.entities.destroy_entity(entity)
         end
      end
   end
   self:_finish_encounter('success')
end

function CollectionQuest:_on_collection_cancelled()
   assert(self._sv.bulletin)

   local bulletin_data = self._sv._info.bulletins.collection_failed

   bulletin_data.ok_callback = '_on_collection_failed_ok'
   self:_update_bulletin(bulletin_data)
end

-- after the user clicks the ok button, transition to the next encounter
-- in the arc (after setting the 'fail' out_edge)
--
function CollectionQuest:_on_collection_failed_ok()
   self:_finish_encounter('fail')
end

function CollectionQuest:_start_source_listener()
   local source = self._sv.source_entity

   assert(source:is_valid())
   assert(not self._source_listener)

   self._source_listener = radiant.events.listen(source, 'radiant:entity:pre_destroy', function()
         local ctx = self._sv.ctx
         ctx.arc:terminate(ctx)
      end)
end

function CollectionQuest:_stop_source_listener()
   if self._source_listener then
      self._source_listener:destroy()
      self._source_listener = nil
   end
end

function CollectionQuest:_cache_player_tracking_data()
   local ctx = self._sv.ctx
   local player_id = ctx.player_id
   local inventory = stonehearth.inventory:get_inventory(player_id)
   local tracker = inventory:get_item_tracker('stonehearth:basic_inventory_tracker')

   self._player_inventory_tracker = tracker
   self._player_inventory_tracking_data = tracker:get_tracking_data()
end

return CollectionQuest

