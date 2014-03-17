
local SleepOnGroundAction = class()
SleepOnGroundAction.name = 'sleep on the cold, hard, unforgiving ground'
SleepOnGroundAction.does = 'stonehearth:sleep_exhausted'
SleepOnGroundAction.args = {}
SleepOnGroundAction.version = 2
SleepOnGroundAction.priority = 1

function SleepOnGroundAction:run(ai, entity)
   ai:execute('stonehearth:drop_carrying_now')
   ai:execute('stonehearth:run_effect', { effect = 'yawn' })
   ai:execute('stonehearth:run_effect', { effect = 'sit_on_ground' })

   --When sleeping, we have a small chance of dreaming
   --TODO: different dreams for sleeping on the ground?
   radiant.events.trigger(stonehearth.personality, 'stonehearth:journal_event', 
                         {entity = entity, description = 'dreams'})


   -- goto sleep
   radiant.entities.think(entity, '/stonehearth/data/effects/thoughts/sleepy')
   radiant.entities.add_buff(entity, 'stonehearth:buffs:sleeping');

   ai:execute('stonehearth:run_effect_timed', { effect = 'sleep', duration = '1h'})
   radiant.entities.set_attribute(entity, 'sleepiness', 0)
   radiant.entities.add_buff(entity, 'stonehearth:buffs:groggy')
end

function SleepOnGroundAction:stop(ai, entity)
   -- xxx, localize
   local name = radiant.entities.get_display_name(entity)
   stonehearth.events:add_entry(name .. ' awakes groggy from sleeping on the cold, hard, unforgiving earth.', 'warning')
   radiant.entities.unthink(entity, '/stonehearth/data/effects/thoughts/sleepy')
   radiant.entities.remove_buff(entity, 'stonehearth:buffs:sleeping');   
end

return SleepOnGroundAction

