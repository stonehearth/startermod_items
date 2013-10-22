--[[
   This action compels a person to do a function.
   Use with care so they don't conflict!
]]

local CompelToAction = class()

CompelToAction.name = 'stonehearth:actions:compel_to_action'
CompelToAction.does = 'stonehearth:top'
CompelToAction.priority = 0

function CompelToAction:__init(ai, entity)
   self._entity = entity
   self._ai = ai
   self._target_action = {}
   self._talisman = nil
   -- xxx, this is bad. it should not be a "global" event
   radiant.events.listen(radiant.events, 'compulsion_event', self, self.on_compulsion)
end

--[[
   Call the event with the function to execute when this runs.
   If there is a target person, check if this applies to us.
   TODO: ability to pass in a set of people to check against
   Pass in any event_data needed
]]
function CompelToAction:on_compulsion(e)

   self._target_action = e.action
   self._talisman = e.talisman

   local entity_id = self._entity:get_id()
   local target_person_entity_id = e.entity:get_id()
   if entity_id and target_person_entity_id and entity_id ~= target_person_entity_id then
      return
   end
   self._ai:set_action_priority(self, 10)
end

--[[
  The function must take the ai and entity parameters, so the entity can move.
  Optionally, if the user passed in action_data, send it along too.
]]
function CompelToAction:run(ai, entity)
   ai:execute(self._target_action.does, self._talisman)
   self._ai:set_action_priority(self, 0)
end

function CompelToAction:stop(ai,entity)
end

return CompelToAction