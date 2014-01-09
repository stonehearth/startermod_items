--[[
   Call this action when a user clicks on a tool
   and then clicks on that person to take up that profession.
]]
local personality_service = require 'services.personality.personality_service'

local GrabTalismanAction = class()

GrabTalismanAction.name = 'grab talisman'
GrabTalismanAction.does = 'stonehearth:top'
GrabTalismanAction.version = 1
GrabTalismanAction.priority = 0

--[[
   Usually created by a compel action, since we don't want everyone
   listening all the time for whether they're about to be promoted,
   and since we want people to drop whatever they're doing in
   order to get their new profession.
]]
function GrabTalismanAction:__init(ai, entity)
   radiant.check.is_entity(entity)
   self._entity = entity
   self._ai = ai
   
   radiant.events.listen(entity, 'stonehearth:grab_talisman', self, self.grab_talisman)
   radiant.events.listen(entity, 'stonehearth:on_effect_trigger', self, self.on_effect_trigger)
end

function GrabTalismanAction:grab_talisman(e)
   self._talisman_entity = e.talisman
   self._ai:set_action_priority(self, 100)
end
--[[
   When the action comes up on the priority lottery,
   go to the item, play the animation, and promote self to
   the target profession. If someone else is promoting themselves
   at the same time, the animation may complete but the promotion
   won't happen.
]]
function GrabTalismanAction:run(ai, entity)
   assert(self._talisman_entity)

   --Am I carrying anything? If so, drop it
   local drop_location = radiant.entities.get_world_grid_location(entity)
   ai:execute('stonehearth:drop_carrying', drop_location)
   
   --The talisman might disappear if someone destroys the workshop or some other
   --person ninja-promotes themselves. So double check!
   local talisman_component = self._talisman_entity:get_component('stonehearth:promotion_talisman')
   if not talisman_component then
      ai:abort()
   end

   local workbench_entity = talisman_component:get_workshop():get_entity();

   --TODO: if the dude is currently not a worker, he should drop his talisman
   --and/or tell his place of work to spawn a new one. (Disassociate worker from queue?)

   ai:execute('stonehearth:goto_entity', workbench_entity)
   radiant.entities.remove_child(workbench_entity, self._talisman_entity)
   ai:execute('stonehearth:run_effect', 'promote', nil, {talisman = self._talisman_entity})

   --Remove the entity from the world, if it still exists
   if radiant.check.is_entity(self._talisman_entity) then
      radiant.entities.destroy_entity(self._talisman_entity)
   end
   self._ai:set_action_priority(self, 0)
end

--[[
   The trigger track fires the on_trigger event that causes us to change
   the worker's clothes. We should still check the
   the entity in question is really our entity in case 2 prmotions happen
   at the exact same moment.
]]
function GrabTalismanAction:on_effect_trigger(e)
   local info = e.info
   local effect = e.effect

   if e.info.info.event == "change_outfit" and self._talisman_entity and self._talisman_entity:get_component('stonehearth:promotion_talisman') then
      --Remove the current class for the person
      local current_profession_script = self._entity:get_component('stonehearth:profession'):get_script()
      local current_profession_script_api = radiant.mods.load_script(current_profession_script)
      if current_profession_script_api.demote then
         current_profession_script_api.demote(self._entity)
      end

      --Add the new class
      local promotion_talisman_component = self._talisman_entity:get_component('stonehearth:promotion_talisman')
      local script = promotion_talisman_component:get_script()
      radiant.mods.load_script(script).promote(self._entity, promotion_talisman_component:get_workshop())

      --Log in personal event log
      local activity_name = radiant.entities.get_entity_data(self._talisman_entity, 'stonehearth:activity_name')
      if activity_name then
         radiant.events.trigger(personality_service, 'stonehearth:journal_event', 
                                {entity = self._entity, description = activity_name})
      end
   end
end

function GrabTalismanAction:stop()
end

return GrabTalismanAction