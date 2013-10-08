--[[
   This action compels a person to do a function.
   Use with care so they don't conflict!
]]

local CompelToAction = class()

CompelToAction.name = 'stonehearth:actions:compel_to_action'
CompelToAction.does = 'stonehearth:top'
CompelToAction.priority = 0

radiant.events.register_event('stonehearth:events:compulsion_event')

function CompelToAction:__init(ai, entity)
   self._entity = entity
   self._ai = ai
   self._target_action = {}
   self._action_data = nil
   radiant.events.listen('stonehearth:events:compulsion_event', self)
end

--[[
   Call the event with the function to execute when this runs.
   If there is a target person, check if this applies to us.
   TODO: ability to pass in a set of people to check against
   Pass in any event_data needed
]]
CompelToAction['stonehearth:events:compulsion_event'] = function(self, target_action, target_person_entity, action_data)
   local entity_id = self._entity:get_id()
   local target_person_entity_id = target_person_entity:get_id()
   if entity_id and target_person_entity_id and entity_id ~= target_person_entity_id then
      return
   end
   self._target_action = target_action
   self._action_data = action_data
   self._ai:set_action_priority(self, 10)
end

--[[
  The function must take the ai and entity parameters, so the entity can move.
  Optionally, if the user passed in action_data, send it along too.
]]
function CompelToAction:run(ai, entity)
   ai:execute(self._target_action.does, self._action_data)
   self._ai:set_action_priority(self, 0)
end

function CompelToAction:stop(ai,entity)
end

return CompelToAction