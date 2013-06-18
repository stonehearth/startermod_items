--[[
   This action compels a person to do a function.
   Use with care so they don't conflict!
]]

local CompelToAction = class()

CompelToAction.name = 'stonehearth.actions.compel_to_action'
CompelToAction.does = 'stonehearth.activities.top'
CompelToAction.priority = 0

radiant.events.register_event('stonehearth.events.compulsion_event')

function CompelToAction:__init(ai, entity)
   self._ai = ai
   self._target_action = {}
   radiant.events.listen('stonehearth.events.compulsion_event', self)
end

--[[
   Call the event with the function to execute when this runs. 
]]
CompelToAction['stonehearth.events.compulsion_event'] = function(self, target_action)
   self._target_action = target_action
   self._ai:set_action_priority(self, 10)
end

--[[
  The function must take the ai and entity parameters, so the entity can move.
]]
function CompelToAction:run(ai, entity)
   self._target_action(ai, entity)
   self._ai:set_action_priority(self, 0)
end

function CompelToAction:stop(ai,entity)
end

return CompelToAction