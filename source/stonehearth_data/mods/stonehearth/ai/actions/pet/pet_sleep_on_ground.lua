local PetSleepOnGround = class()
PetSleepOnGround.name = 'critter sleep on ground'
PetSleepOnGround.does = 'stonehearth:sleep_exhausted'
PetSleepOnGround.args = {}
PetSleepOnGround.version = 2
PetSleepOnGround.priority = 1

function PetSleepOnGround:run(ai, entity)
   ai:execute('stonehearth:run_effect', { effect = 'sit_on_ground' })
   ai:execute('stonehearth:run_sleep_effect', { duration_string = '1h' })

   radiant.entities.set_attribute(entity, 'sleepiness', stonehearth.constants.sleep.MIN_SLEEPINESS)
end

function PetSleepOnGround:stop(ai, entity)
   -- xxx, localize
   local name = radiant.entities.get_display_name(entity)
end

return PetSleepOnGround
