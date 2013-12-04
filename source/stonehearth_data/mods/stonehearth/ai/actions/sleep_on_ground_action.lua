local event_service = require 'services.event.event_service'

local SleepOnGroundAction = class()

SleepOnGroundAction.name = 'sleep on the cold, hard, unforgiving ground'
SleepOnGroundAction.does = 'stonehearth:sleep_on_ground'
SleepOnGroundAction.priority = 1


function SleepOnGroundAction:__init(ai, entity)
   self._entity = entity
   self._ai = ai
end

function SleepOnGroundAction:run(ai, entity, bed, path)
   --Am I carrying anything? If so, drop it
   local drop_location = radiant.entities.get_world_grid_location(entity)
   radiant.entities.add_buff(self._entity, 'stonehearth:buffs:sleeping');
      
   ai:execute('stonehearth:drop_carrying', drop_location)
   ai:execute('stonehearth:run_effect', 'yawn')
   ai:execute('stonehearth:run_effect', 'sit_on_ground')
   ai:execute('stonehearth:run_effect', 'sleep')

end

function SleepOnGroundAction:stop()
   radiant.entities.remove_buff(self._entity, 'stonehearth:buffs:sleeping');
   radiant.entities.add_buff(self._entity, 'stonehearth:buffs:groggy');
   radiant.entities.unthink(self._entity, '/stonehearth/data/effects/thoughts/sleepy')

   -- xxx, localize
   local name = radiant.entities.get_display_name(self._entity)
   event_service:add_entry(name .. ' awakes groggy from sleeping on the cold, hard, unforgiving earth.', 'warning')
end

return SleepOnGroundAction
