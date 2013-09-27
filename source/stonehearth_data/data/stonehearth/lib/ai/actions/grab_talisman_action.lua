--[[
   Call this action when a user clicks on a tool
   and then clicks on that person to take up that profession.
]]

local GrabTalismanAction = class()

GrabTalismanAction.name = 'stonehearth.actions.check_craftable'
GrabTalismanAction.does = 'stonehearth.grab_talisman'
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
   radiant.events.listen('radiant.animation.on_trigger', self)
end

--[[
   When the action comes up on the priority lottery,
   go to the item, play the animation, and promote self to
   the target profession.
]]
function GrabTalismanAction:run(ai, entity, action_data)
   if action_data and action_data.talisman then
      self._talisman_entity = action_data.talisman
      local promotion_info_component = self._talisman_entity:get_component('stonehearth:talisman_promotion_info')
      local workbench_entity = promotion_info_component:get_promotion_data().workshop:get_entity();

      --TODO: if the dude is currently not a worker, he should drop his talisman
      --and/or tell his place of work to spawn a new one. (Disassociate worker from queue?)

      ai:execute('stonehearth.goto_entity', workbench_entity)
      radiant.entities.remove_child(workbench_entity, self._talisman_entity)
      ai:execute('stonehearth.run_effect', 'promote', nil, {talisman = self._talisman_entity})

      --Remove the entity from the world
      radiant.entities.destroy_entity(self._talisman_entity)
   end
end

--[[
   The trigger track fires the on_trigger event that causes us to change
   the worker's clothes. We should still check the
   the entity in question is really our entity in case 2 prmotions happen
   at the exact same moment.
]]
GrabTalismanAction['radiant.animation.on_trigger'] = function(self, info, effect, entity)
   local entity_id = self._entity:get_id()
   if (entity_id ~= entity:get_id()) or not self._talisman_entity then
      return
   end

   --TODO: BUG somewhere, this should only be the info inside the event, not the whole event blob
   for name, e in pairs(info) do
      if name == "info" then
         if e.event == "change_outfit" then
            --Time to remove the old class

            local old_job_info = entity:get_component('stonehearth:job_info')
            local old_class_script = old_job_info:get_class_script()
            local old_class_script_api = radiant.mods.load_script(old_class_script)
            if old_class_script_api.demote then
               old_class_script_api.demote(self._entity)
            end

            --Time to add the new class
            local talisman_profession_info = self._talisman_entity:get_component('stonehearth:talisman_promotion_info')
            local class_script = talisman_profession_info:get_class_script()
            radiant.mods.load_script(class_script).promote(self._entity, talisman_profession_info:get_promotion_data())
         end
      end
   end
end

function GrabTalismanAction:stop()
end

return GrabTalismanAction