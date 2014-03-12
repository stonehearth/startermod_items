local PetSleepOnGround = class()
PetSleepOnGround.name = 'critter sleep on ground'
PetSleepOnGround.does = 'stonehearth:sleep_exhausted'
PetSleepOnGround.args = {}
PetSleepOnGround.version = 2
PetSleepOnGround.priority = 1

-- TODO: refactor this with sleep_on_ground and sleep_in_bed-adjacent
function PetSleepOnGround:run(ai, entity)
   --TODO: does the critter have a sit action?
   ai:execute('stonehearth:run_effect', { effect = 'sit_on_ground' })

   -- goto sleep
   radiant.entities.think(entity, '/stonehearth/data/effects/thoughts/sleepy')
   radiant.entities.add_buff(entity, 'stonehearth:buffs:sleeping');

   ai:execute('stonehearth:run_effect_timed', { effect = 'sleep', duration = 1})
   radiant.entities.set_attribute(entity, 'sleepiness', 0)
end

function PetSleepOnGround:stop(ai, entity)
   -- xxx, localize
   local name = radiant.entities.get_display_name(entity)
   radiant.entities.unthink(entity, '/stonehearth/data/effects/thoughts/sleepy')
   radiant.entities.remove_buff(entity, 'stonehearth:buffs:sleeping');   
end

return PetSleepOnGround
