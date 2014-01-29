
local SleepOnGroundAction = class()
SleepOnGroundAction.name = 'sleep on the cold, hard, unforgiving ground'
SleepOnGroundAction.does = 'stonehearth:sleep_exhausted'
SleepOnGroundAction.args = {}
SleepOnGroundAction.version = 2
SleepOnGroundAction.priority = 1

function SleepOnGroundAction:__init(ai, entity)
   self._entity = entity
   self._ai = ai
end

function SleepOnGroundAction:run(ai, entity)
   ai:execute('stonehearth:drop_carrying_now')
   ai:execute('stonehearth:run_effect', { effect = 'yawn' })
   ai:execute('stonehearth:run_effect', { effect = 'sit_on_ground' })

   --When sleeping, we have a small chance of dreaming
   --TODO: different dreams for sleeping on the ground?
   radiant.events.trigger(stonehearth.personality, 'stonehearth:journal_event', 
                         {entity = self._entity, description = 'dreams'})

   ai:execute('stonehearth:run_effect_timed', { effect = 'sleep', duration = 1})
   radiant.entities.set_attribute(entity, 'sleepiness', 0)
end

function SleepOnGroundAction:stop(ai, entity)
   -- xxx, localize
   local name = radiant.entities.get_display_name(entity)
   stonehearth.events:add_entry(name .. ' awakes groggy from sleeping on the cold, hard, unforgiving earth.', 'warning')
end

return SleepOnGroundAction

