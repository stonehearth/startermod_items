local constants = require('constants')

local AddSleepBuff = class()
AddSleepBuff.name = 'add sleep buff'
AddSleepBuff.does = 'stonehearth:add_sleep_buff'
AddSleepBuff.args = { }
AddSleepBuff.version = 2
AddSleepBuff.priority = 1

function AddSleepBuff:start(ai, entity)
   -- this is somewhat broken.  buffs should be refcounted, right?  stop is going to
   -- remove it uncondtionally...
   if not radiant.entities.has_buff(entity, 'stonehearth:buffs:sleeping') then
      radiant.entities.add_buff(entity, 'stonehearth:buffs:sleeping');
   end
end

function AddSleepBuff:stop(ai, entity)
   radiant.entities.remove_buff(entity, 'stonehearth:buffs:sleeping');
end

return AddSleepBuff
