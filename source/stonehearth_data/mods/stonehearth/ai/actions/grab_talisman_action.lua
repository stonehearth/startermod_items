--[[
   Call this action when a user clicks on a tool
   and then clicks on that person to take up that profession.
]]

local GrabTalismanAction = class()

GrabTalismanAction.name = 'grab talisman'
GrabTalismanAction.does = 'stonehearth:grab_talisman'
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
   -- xxx inkblot, find out how to wire this up correctly
   radiant.events.listen(radiant.events, 'on_trigger', self, self.on_trigger)
end

--[[
   When the action comes up on the priority lottery,
   go to the item, play the animation, and promote self to
   the target profession.
]]
function GrabTalismanAction:run(ai, entity, talisman)
   if talisman then
      self._talisman_entity = talisman
      local workbench_entity = self._talisman_entity:get_component('stonehearth:promotion_talisman'):get_workshop():get_entity();

      --TODO: if the dude is currently not a worker, he should drop his talisman
      --and/or tell his place of work to spawn a new one. (Disassociate worker from queue?)

      ai:execute('stonehearth:goto_entity', workbench_entity)
      radiant.entities.remove_child(workbench_entity, self._talisman_entity)
      ai:execute('stonehearth:run_effect', 'promote', nil, {talisman = self._talisman_entity})

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
function GrabTalismanAction:on_trigger(e)
   local info = e.info
   local effect = e.effect
   local entity = e.entity

   local entity_id = self._entity:get_id()
   if (entity_id ~= entity:get_id()) or not self._talisman_entity then
      return
   end

   --TODO: BUG somewhere, this should only be the info inside the event, not the whole event blob
   for name, e in pairs(info) do
      if name == "info" then
         if e.event == "change_outfit" then

            --Remove the current class for the person
            local current_profession_script = entity:get_component('stonehearth:profession'):get_script()
            local current_profession_script_api = radiant.mods.load_script(current_profession_script)
            if current_profession_script_api.demote then
               current_profession_script_api.demote(self._entity)
            end

            --Add the new class
            local promotion_talisman_component = self._talisman_entity:get_component('stonehearth:promotion_talisman')
            local script = promotion_talisman_component:get_script()
            radiant.mods.load_script(script).promote(self._entity, promotion_talisman_component:get_workshop())
         end
      end
   end
end

function GrabTalismanAction:stop()
end

return GrabTalismanAction