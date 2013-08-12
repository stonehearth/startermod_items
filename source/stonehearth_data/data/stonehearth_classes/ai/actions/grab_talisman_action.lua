--[[
   This action fires on a person when a user clicks on a tool
   and then clicks on that person to take up that profession.
]]

local GrabTalismanAction = class()

GrabTalismanAction.name = 'stonehearth.actions.check_craftable'
GrabTalismanAction.does = 'stonehearth.activities.top'
GrabTalismanAction.priority = 0

radiant.events.register_event('stonehearth_item.promote_citizen')

function GrabTalismanAction:__init(ai, entity)
   radiant.check.is_entity(entity)
   self._entity = entity         --the dude to be promoted
   self._ai = ai

   radiant.events.listen('stonehearth_item.promote_citizen', self)
   radiant.events.listen('radiant.animation.on_trigger', self)
end

--[[
   When the button is clicked, check if entity is the target.
   If so, tell the target to go run over and promote himself.
   talisman: the entity that is going to promote the person
   target_person: the person who should run over to be promoted
]]
GrabTalismanAction['stonehearth_item.promote_citizen'] = function(self, event_data)
   --If we aren't the person targeted, then just return
   --TODO: should we have a function to get this?
   local entity_id = self._entity:get_id()
   if entity_id ~= event_data.target_person then
      return
   end
   self._talisman_entity = event_data.talisman
   self._ai:set_action_priority(self, 5)
end

--[[
   When the action comes up on the priority lottery,
   go to the item, play the animation, and promote self to
   the target profession.
]]
function GrabTalismanAction:run(ai, entity)
   radiant.events.unlisten('stonehearth_item.promote_citizen', self)

   --TODO: if the dude is currently not a worker, he should drop his talisman
   --and/or tell his place of work to spawn a new one. (Disassociate worker from queue?)

   ai:execute('stonehearth.activities.goto_entity', self._talisman_entity)
   radiant.entities.remove_child(radiant._root_entity, self._talisman_entity)
   ai:execute('stonehearth.activities.run_effect', 'promote', nil, {talisman = self._talisman_entity})

   --Remove the entity from the world
   radiant.entities.destroy_entity(self._talisman_entity)
end

--The promote action has events inside of it
--TODO: on what thread are they called?
GrabTalismanAction['radiant.animation.on_trigger'] = function(self, info, effect, entity)
   local entity_id = self._entity:get_id()
   if entity_id ~= entity:get_id() then
      return
   end
   --TODO: this is the only way I could find to easily extract the contents of the info object
   for name, e in radiant.resources.pairs(info) do
      if name == "info" then
         if e.event == "change_outfit" then
            local talisman_profession_info = self._talisman_entity:get_component('stonehearth_classes:profession_info')
            local profession_name = talisman_profession_info:get_profession()
            radiant.mods.load_api('/stonehearth_' .. profession_name .. '_class/').promote(self._entity, talisman_profession_info)
         end
      end
   end
end

--[[
   Always called after run, or if the action is interrupted
]]
function GrabTalismanAction:stop()
   self._ai:set_action_priority(self, 0)
   radiant.events.listen('stonehearth_item.promote_citizen', self)
end

return GrabTalismanAction