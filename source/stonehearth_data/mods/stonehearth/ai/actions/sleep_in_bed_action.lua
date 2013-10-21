--[[
   SleepInBedAction handles the specifics of how to go to sleep once
   a bed has been found. Finding a bed and managing all state related to
   sleeping is done in sleep_action
--]]
local SleepInBedAction = class()

SleepInBedAction.name = 'sleep in a bed'
SleepInBedAction.does = 'stonehearth:sleep_in_bed'
SleepInBedAction.priority = 1


function SleepInBedAction:__init(ai, entity)
   self._entity = entity         --the game character
   self._ai = ai
   self._bed = nil
end

--[[
   Follow the path to the bed, then play the sleep-related animations.
--]]
function SleepInBedAction:run(ai, entity, bed, path)
   -- renew our lease on the bed.

   -- Mark the bed as being used
   self._bed = bed

   -- If the bed moves or is destroyed between now and before the sleeper wakes up, just
   -- go ahead and abort.
   -- TODO: right now, moving destroys, instead of actually moving the instance, fix
   -- appear/disappear bug
   self._bed_moved_promise = radiant.entities.on_entity_moved(bed, function()
      ai:abort()
   end);
   radiant.entities.on_destroy(bed, function()
      self._bed = nil
      local bed_lease = self._entity:get_component('stonehearth:bed_lease')
      bed_lease:set_bed(nil)
      ai:abort()
   end);
   
   --Am I carrying anything? If so, drop it
   local drop_location = radiant.entities.get_world_grid_location(entity)
   ai:execute('stonehearth:drop_carrying', drop_location)

   -- walk over to the bed
   ai:execute('stonehearth:follow_path', path)

   -- get ready to go to sleep
   ai:execute('stonehearth:run_effect', 'yawn')

   -- move directly on top of the bed
   local bed_location = radiant.entities.get_world_grid_location(bed)

   bed_location.y = bed_location.y + 1
   bed_location.z = bed_location.z + 1
   radiant.entities.move_to(entity, bed_location)
   radiant.entities.turn_to(entity, 180) -- xxx, when beds can be placed at aritrary rotations this will break

   -- goto sleep
   ai:execute('stonehearth:run_effect', 'goto_sleep')
   ai:execute('stonehearth:run_effect', 'sleep')
end

function SleepInBedAction:stop()
   if self._bed_moved_promise then
      self._bed_moved_promise:destroy()
      self._bed_moved_promise = nil
   end
   --Sleeping seems to make us 1 higher than we should be
   
end

return SleepInBedAction
