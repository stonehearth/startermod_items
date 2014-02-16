local CritterSleepOnGround = class()
CritterSleepOnGround.name = 'critter sleep on ground'
CritterSleepOnGround.does = 'stonehearth:sleep_exhausted'
CritterSleepOnGround.args = {}
CritterSleepOnGround.version = 2
CritterSleepOnGround.priority = 1


function CritterSleepOnGround:run(ai, entity)
   --TODO: does the critter have a sit action?
   ai:execute('stonehearth:run_effect', { effect = 'sit_on_ground' })

   -- goto sleep
   radiant.entities.think(entity, '/stonehearth/data/effects/thoughts/sleepy')
   radiant.entities.add_buff(entity, 'stonehearth:buffs:sleeping');

   ai:execute('stonehearth:run_effect_timed', { effect = 'sleep', duration = 1})
   radiant.entities.set_attribute(entity, 'sleepiness', 0)
end

function CritterSleepOnGround:stop(ai, entity)
   -- xxx, localize
   local name = radiant.entities.get_display_name(entity)
   radiant.entities.unthink(entity, '/stonehearth/data/effects/thoughts/sleepy')
   radiant.entities.remove_buff(entity, 'stonehearth:buffs:sleeping');   
end

return CritterSleepOnGround

