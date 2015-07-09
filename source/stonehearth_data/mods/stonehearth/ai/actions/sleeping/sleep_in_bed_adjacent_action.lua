local SleepLib = require 'ai.lib.sleep_lib'
local Entity = _radiant.om.Entity

local SleepInBedAdjacent = class()

SleepInBedAdjacent.name = 'sleep in bed adjacent'
SleepInBedAdjacent.status_text = 'sleeping...'
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

   local mount_component = bed:add_component('stonehearth:mount')
   mount_component:mount(entity)

   ai:execute('stonehearth:run_effect', { effect = 'goto_sleep' })

   radiant.events.trigger_async(entity, 'stonehearth:sleep_in_bed', { bed_uri = bed:get_uri() })

   local sleep_duration, rested_sleepiness = SleepLib.get_sleep_parameters(entity, bed)
   ai:execute('stonehearth:run_sleep_effect', { duration_string = sleep_duration })
   radiant.entities.set_attribute(entity, 'sleepiness', rested_sleepiness)
end

function SleepInBedAdjacent:stop(ai, entity, args)
   local bed = args.bed

   if bed:is_valid() then
      local mount_component = bed:add_component('stonehearth:mount')
      if mount_component:is_in_use() then
         assert(mount_component:get_user() == entity)
         mount_component:dismount()
      end
   end
end

return SleepInBedAdjacent
