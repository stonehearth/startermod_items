--[[
   SleepInBedAction handles the specifics of how to go to sleep once
   a bed has been found. Finding a bed and managing all state related to
   sleeping is done in sleep_action
--]]
local SleepInBedAction = class()

SleepInBedAction.name = 'stonehearth.actions.sleep_in_bed'
SleepInBedAction.does = 'stonehearth.sleep_in_bed'
SleepInBedAction.priority = 1


function SleepInBedAction:__init(ai, entity)
   self._entity = entity         --the game character
   self._ai = ai
end

--[[
   Follow the path to the bed, then play the sleep-related animations.
--]]
function SleepInBedAction:run(ai, entity, bed, path)
   -- renew our lease on the bed.

   --Am I carrying anything? If so, drop it
   local drop_location = path:get_source_point_of_interest()
   ai:execute('stonehearth.drop_carrying', drop_location)

   -- walk over to the bed
   ai:execute('stonehearth.follow_path', path)

   -- get ready to go to sleep
   ai:execute('stonehearth.run_effect', 'yawn')

   -- move directly on top of the bed
   local bed_location = radiant.entities.get_world_grid_location(bed)

   bed_location.y = bed_location.y + 1
   bed_location.z = bed_location.z + 1
   radiant.entities.move_to(entity, bed_location)
   radiant.entities.turn_to(entity, 180) -- xxx, when beds can be placed at aritrary rotations this will break

   -- goto sleep
   ai:execute('stonehearth.run_effect', 'goto_sleep')
   ai:execute('stonehearth.run_effect', 'sleep')
end

return SleepInBedAction
