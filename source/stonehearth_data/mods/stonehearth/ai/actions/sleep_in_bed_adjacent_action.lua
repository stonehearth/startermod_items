local Entity = _radiant.om.Entity

local SleepInBedAdjacent = class()
SleepInBedAdjacent.name = 'sleep in bed adjacent'
SleepInBedAdjacent.does = 'stonehearth:sleep_in_bed_adjacent'
SleepInBedAdjacent.args = {
   bed = Entity,     -- the bed to sleep in
}
SleepInBedAdjacent.version = 2
SleepInBedAdjacent.priority = 1

function SleepInBedAdjacent:start(ai, entity, args)
   local bed = args.bed
   local lease_component = bed:add_component('stonehearth:lease')
   if not lease_component:acquire('stonehearth:bed', entity) then
      ai:abort('could not lease %s (%s has it).', tostring(bed),
               tostring(lease_component:get_owner('stonehearth:bed')))
   end
end

--[[
   Follow the path to the bed, then play the sleep-related animations.
--]]
function SleepInBedAdjacent:run(ai, entity, args)
   local bed = args.bed
   
   ai:execute('stonehearth:run_effect', { effect = 'yawn' })

   --When sleeping, we have a small chance of dreaming
   radiant.events.trigger(stonehearth.personality, 'stonehearth:journal_event', 
                         {entity = entity, description = 'dreams'})

   -- move directly on top of the bed
   self._restore_location = radiant.entities.get_world_grid_location(entity)
   local bed_location = radiant.entities.get_world_grid_location(bed)
   bed_location.y = bed_location.y + 1
   radiant.entities.move_to(entity, bed_location)

   local q = bed:get_component('mob'):get_rotation()
   entity:get_component('mob'):set_rotation(q)

   -- goto sleep  
   radiant.entities.think(entity, '/stonehearth/data/effects/thoughts/sleepy',  stonehearth.constants.think_priorities.SLEEPY)
   radiant.entities.add_buff(entity, 'stonehearth:buffs:sleeping');

   ai:execute('stonehearth:run_effect', { effect = 'goto_sleep' })
   ai:execute('stonehearth:run_effect_timed', { effect = 'sleep', duration = '1h'})
   radiant.entities.set_attribute(entity, 'sleepiness', 0)
end

function SleepInBedAdjacent:stop(ai, entity, args)
   if self._restore_location then
      radiant.entities.move_to(entity, self._restore_location)
      self._restore_location = nil
   end
   radiant.entities.unthink(entity, '/stonehearth/data/effects/thoughts/sleepy')
   radiant.entities.remove_buff(entity, 'stonehearth:buffs:sleeping');   
end

return SleepInBedAdjacent
