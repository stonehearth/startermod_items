local CollectStartingResourcesScenario = class()
local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_rng()

function CollectStartingResourcesScenario:initialize()
   self._sv.player_id = 'player_1'
   self._sv.item_count = 0

   self:_load_data()
end

function CollectStartingResourcesScenario:_load_data()
  self._scenario_data = radiant.resources.load_json('stonehearth:quests:collect_starting_resources').scenario_data
end

function CollectStartingResourcesScenario:start()
   self._inventory = stonehearth.inventory:get_inventory(self._sv.player_id)

   self._item_added_listener = radiant.events.listen(self._inventory, 'stonehearth:item_added', self, self._on_inventory_item_added)
   self._item_removed_listener = radiant.events.listen(self._inventory, 'stonehearth:item_removed', self, self._on_inventory_item_removed)

   --Send the notice to the bulletin service.
   self._sv.bulletin = stonehearth.bulletin_board:post_bulletin(self._sv.player_id)
     :set_ui_view('StonehearthQuestBulletinDialog')
     :set_callback_instance(self)
     :set_close_on_handle(false)
     :set_data({
         title = self._scenario_data.title,
         progress = self:_get_progress(),
         message = self._scenario_data.message,
         rewards = self._scenario_data.rewards,
         accepted_callback = "_on_accepted",
         declined_callback = "_on_declined",
     })
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
   self._sv.bulletin:modify_data({
      progress = self:_get_progress(),
   })

   if self._sv.item_count >= 20 then
     self:_complete_quest()
   end
end

function CollectStartingResourcesScenario:_get_progress()
   return 'Wooden logs stored: ' .. self._sv.item_count .. '/20'
end

function CollectStartingResourcesScenario:_on_accepted()
  -- do nothing
end

function CollectStartingResourcesScenario:_complete_quest()
   -- notify the UI somehow

   -- add town exp
   local town = stonehearth.town:get_town(self._sv.player_id)
   town:add_exp(self._scenario_data.rewards.exp.value)
end

function CollectStartingResourcesScenario:_on_declined()
  self:destroy()
end

function CollectStartingResourcesScenario:destroy()
   if self._item_added_listener then
      self._item_added_listener:destroy()
    self._item_added_listener = nil
   end

   if self._item_removed_listener then
      self._item_removed_listener:destroy()
      self._item_removed_listener = nil
   end

   local bulletin_id = self._sv.bulletin:get_id()
   stonehearth.bulletin_board:remove_bulletin(bulletin_id)
   self._sv.bulletin = nil
end

return CollectStartingResourcesScenario
