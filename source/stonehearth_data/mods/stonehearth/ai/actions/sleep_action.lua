local constants = require('constants')

local SleepAction = class()
SleepAction.name = 'goto sleep'
SleepAction.does = 'stonehearth:top'
SleepAction.args = { }
SleepAction.version = 2
SleepAction.priority = constants.priorities.needs.SLEEP

function SleepAction:start(ai, entity)
   -- this is somewhat broken.  buffs should be refcounted, right?  stop is going to
   -- remove it uncondtionally...
   if not radiant.entities.has_buff(entity, 'stonehearth:buffs:sleeping') then
      radiant.entities.add_buff(entity, 'stonehearth:buffs:sleeping');
   end
end

function SleepAction:stop(ai, entity)
   radiant.entities.remove_buff(entity, 'stonehearth:buffs:sleeping');
end

local ai = stonehearth.ai
return ai:create_compound_action(SleepAction)
         :execute('stonehearth:sleep')
