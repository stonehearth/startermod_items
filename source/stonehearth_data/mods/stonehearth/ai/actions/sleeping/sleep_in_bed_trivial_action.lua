local Entity = _radiant.om.Entity
local SleepInBedTrivial = class()

SleepInBedTrivial.name = 'sleep in bed trivial'
SleepInBedTrivial.does = 'stonehearth:sleep'
SleepInBedTrivial.args = { }
SleepInBedTrivial.version = 2
SleepInBedTrivial.priority = 1

function SleepInBedTrivial:start_thinking(ai, entity, args)
   local parent = radiant.entities.get_parent(entity)
   if not parent then
      return
   end

   if radiant.entities.get_entity_data(parent, 'stonehearth:bed') then
      -- hey, we're already in bed!
      local bed = parent
      local mount_component = bed:add_component('stonehearth:mount')
      assert(mount_component:get_user() == entity)

      ai:set_think_output({ bed = bed })
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(SleepInBedTrivial)
         :execute('stonehearth:reserve_entity', { entity = ai.PREV.bed })
         :execute('stonehearth:sleep_in_current_bed', { bed = ai.BACK(2).bed })
