local Entity = _radiant.om.Entity
local SleepInNewBed = class()

SleepInNewBed.name = 'sleep in new bed'
SleepInNewBed.does = 'stonehearth:sleep'
SleepInNewBed.args = { }
SleepInNewBed.version = 2
SleepInNewBed.priority = 1

function currently_has_bed(entity)
   -- if we don't own any leases, clearly we don't have a bed yet.  
   local lease_component = entity:get_component('stonehearth:lease_holder')
   if not lease_component then
      return false
   end
   -- if the bed that we've leased is still valid, we should not try
   -- to look for a new bed.
   local bed = lease_component:get_first_lease('stonehearth:bed')
   return bed and bed:is_valid()
end

function is_available_bed(target, ai, entity)
   -- make sure it's a bed and no one else has a lease on it
   if radiant.entities.get_entity_data(target, 'stonehearth:bed') then
      local lease_component = target:get_component('stonehearth:lease')
      if not lease_component then
         return true
      end
      return lease_component:can_acquire('stonehearth:bed', entity)
   end
   return false
end

local ai = stonehearth.ai
return ai:create_compound_action(SleepInNewBed)
         :when(function (ai, entity) return not currently_has_bed(entity) end)
         :execute('stonehearth:drop_carrying_now')
         :execute('stonehearth:goto_entity_type', { filter_fn = is_available_bed, description='untaken_available_bed' })
         :execute('stonehearth:reserve_entity', { entity = ai.PREV.destination_entity })
         :execute('stonehearth:sleep_in_bed_adjacent', { bed = ai.PREV.entity })
