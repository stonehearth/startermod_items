local CollectStartingResourcesScenario = class()
local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_rng()

function CollectStartingResourcesScenario:initialize()
   self._sv.player_id = 'player_1'
   self._sv.item_count = 0

   self:restore()
end

function CollectStartingResourcesScenario:restore()
  self._scenario_data = radiant.resources.load_json('stonehearth:quests:collect_starting_resources').scenario_data
end

function CollectStartingResourcesScenario:start()
   self._inventory = stonehearth.inventory:get_inventory(self._sv.player_id)

   self._item_added_listener = radiant.events.listen(self._inventory, 'stonehearth:inventory:item_added', self, self._on_inventory_item_added)
   self._item_removed_listener = radiant.events.listen(self._inventory, 'stonehearth:inventory:item_removed', self, self._on_inventory_item_removed)

   self:_post_bulletin()
end

function CollectStartingResourcesScenario:_post_bulletin(e)
   --Send the notice to the bulletin service.
   local title = self._scenario_data.title
   
   local bulletin_data = {
         title = self._scenario_data.title,
         progress = self:_get_progress(),
         message = self._scenario_data.message,
         rewards = self._scenario_data.rewards,
         ui = '_'
     }
   
   if not self._sv.quest_completed then
      bulletin_data.accepted_callback = "_on_accepted"
      bulletin_data.declined_callback = "_on_declined"
   else 
      bulletin_data.title = 'Completed: ' .. bulletin_data.title
      bulletin_data.ok_callback = '_on_complete_accepted'
   end

   self._sv.bulletin = stonehearth.bulletin_board:post_bulletin(self._sv.player_id)
     :set_ui_view('StonehearthQuestBulletinDialog')
     :set_callback_instance(self)
     :set_type('quest')
     :set_sticky(true)
     :set_close_on_handle(self._sv.quest_completed)
     :set_data(bulletin_data)

   self.__saved_variables:mark_changed()
end

function CollectStartingResourcesScenario:_on_inventory_item_added(e)
   local item = e.item
   
   if radiant.entities.is_material(item, 'wood resource') then
      self._sv.item_count = self._sv.item_count + 1
      self:_update_quest_progress()
   end
end

function CollectStartingResourcesScenario:_on_inventory_item_removed(e)
   local item = radiant.entities.get_entity(e.item_id)
   
   if radiant.entities.is_material(item, 'wood resource') then
      self._sv.item_count = self._sv.item_count - 1
    self:_update_quest_progress()
   end
end

function CollectStartingResourcesScenario:_update_quest_progress()
   if self._sv.quest_completed then
      return
   end
   
   if self._sv.bulletin ~= nil then
      self._sv.bulletin:modify({
         progress = self:_get_progress(),
      })
   end
   
   if self._sv.item_count >= self._scenario_data.num_items_required then
     self:_complete_quest()
   end
end

function CollectStartingResourcesScenario:_get_progress()
   return 'Wooden logs stored: ' .. self._sv.item_count .. '/' .. self._scenario_data.num_items_required
end

function CollectStartingResourcesScenario:_on_accepted(session, response)
  --[[
   if self._sv.bulletin ~= nil then
      self._sv.bulletin:modify_data({
         ui = self._scenario_data.ui
      })
   end
   ]]

   response:resolve(self._scenario_data.on_accept);
end

function CollectStartingResourcesScenario:_on_complete_accepted()
  self:destroy();
end

function CollectStartingResourcesScenario:_complete_quest()
   self:_remove_listeners()

   -- notify the UI by posting a new bulletin
   self:_destroy_bulletin()

   self._sv.quest_completed = true
   self:_post_bulletin()

   -- add the quest rewards
   --[[
   local town = stonehearth.town:get_town(self._sv.player_id)
   local job_component = town:get_entity():get_component('stonehearth:job')
   job_component:add_exp(self._scenario_data.rewards.exp.value)

   stonehearth.dynamic_scenario:force_spawn_scenario('stonehearth:quests:collect_starting_resources')
   ]]

end

function CollectStartingResourcesScenario:_destroy_bulletin()
   if self._sv.bulletin ~= nil then
      local bulletin_id = self._sv.bulletin:get_id()
      stonehearth.bulletin_board:remove_bulletin(bulletin_id)
      self._sv.bulletin = nil
   end
end

function CollectStartingResourcesScenario:_on_declined()
  self:destroy()
end

function CollectStartingResourcesScenario:_remove_listeners()
   if self._item_added_listener then
      self._item_added_listener:destroy()
      self._item_added_listener = nil
   end

   if self._item_removed_listener then
      self._item_removed_listener:destroy()
      self._item_removed_listener = nil
   end
end

function CollectStartingResourcesScenario:destroy()
   self:_remove_listeners()
   self:_destroy_bulletin()
end

return CollectStartingResourcesScenario

