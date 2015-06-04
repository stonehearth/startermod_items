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
   --[[

   for the moment, let's not put leases on beds.  in the future we'll want
   to support ownership of items, which will probably be done with leases,
   so leave this code hanging around.  for now, though, treat beds as
   communal objects.
   
   local bed = args.bed
   local lease_component = bed:add_component('stonehearth:lease')
   if not lease_component:acquire('stonehearth:bed', entity) then
      ai:abort('could not lease %s (%s has it).', tostring(bed),
               tostring(lease_component:get_owner('stonehearth:bed')))
   end
   ]]
end

--[[
   Follow the path to the bed, then play the sleep-related animations.
--]]
function SleepInBedAdjacent:run(ai, entity, args)
   local bed = args.bed
   
   ai:execute('stonehearth:run_effect', { effect = 'yawn' })

   -- move directly on top of the bed
   self._restore_location = radiant.entities.get_world_grid_location(entity)
   local bed_location = radiant.entities.get_world_grid_location(bed)
   bed_location.y = bed_location.y + 1
   radiant.entities.move_to(entity, bed_location)

   local q = bed:get_component('mob'):get_rotation()
   entity:get_component('mob'):set_rotation(q)

   -- goto sleep  
   radiant.entities.add_buff(entity, 'stonehearth:buffs:sleeping');

   ai:execute('stonehearth:run_effect', { effect = 'goto_sleep' })
   radiant.events.trigger_async(entity, 'stonehearth:sleep_in_bed', { bed_uri = bed:get_uri() })

   -- calculate sleep duration in minutes
   local sleep_duration = 60
   local attributes_component = entity:get_component('stonehearth:attributes')
   
   if attributes_component then
      local sleepiness = attributes_component:get_attribute('sleepiness')
      local stamina = attributes_component:get_attribute('stamina')

      local sleep_duration_attribute = attributes_component:get_attribute('sleep_duration')
      if sleep_duration_attribute then
         sleep_duration = radiant.math.round(sleep_duration_attribute)
      end
   end

   local sleep_duration_string = sleep_duration .. 'm'

   ai:execute('stonehearth:run_effect_timed', { effect = 'sleep', duration = sleep_duration_string})
   radiant.entities.set_attribute(entity, 'sleepiness', 0)
end

function SleepInBedAdjacent:stop(ai, entity, args)
   if self._restore_location then
      radiant.entities.move_to(entity, self._restore_location)
      self._restore_location = nil
   end
   radiant.entities.remove_buff(entity, 'stonehearth:buffs:sleeping');   
end

return SleepInBedAdjacent
