local PromotionScenario = class()
local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_rng()

function PromotionScenario:initialize()
   self._sv.player_id = 'player_1'
   self._sv.item_count = 0

   self:_load_data()
end

function PromotionScenario:_load_data()
  self._scenario_data = radiant.resources.load_json('stonehearth:quests:promote_carpenter').scenario_data
end

function PromotionScenario:start()
   self._inventory = stonehearth.inventory:get_inventory(self._sv.player_id)
   self:_post_bulletin()
end

function PromotionScenario:_post_bulletin(e)
   --Send the notice to the bulletin service.
   local title = self._scenario_data.title
   
   local bulletin_data = {
         title = self._scenario_data.title,
         progress = self._scenario_data.progress,
         message = self._scenario_data.message,
         rewards = self._scenario_data.rewards,
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

function PromotionScenario:_on_accepted(session, response)
   response:resolve(self._scenario_data.on_accept);
end

function PromotionScenario:_on_complete_accepted()
  self:destroy();
end

function PromotionScenario:_complete_quest()
   self:_remove_listeners()

   -- notify the UI by posting a new bulletin
   self:_destroy_bulletin()

   self._sv.quest_completed = true
   self:_post_bulletin()
end

function PromotionScenario:_destroy_bulletin()
   if self._sv.bulletin ~= nil then
      local bulletin_id = self._sv.bulletin:get_id()
      stonehearth.bulletin_board:remove_bulletin(bulletin_id)
      self._sv.bulletin = nil
   end
end

function PromotionScenario:_on_declined()
  self:destroy()
end

function PromotionScenario:destroy()
   self:_destroy_bulletin()
end

return PromotionScenario

