local SleepLib = require 'ai.lib.sleep_lib'
local Point3 = _radiant.csg.Point3

local SleepTrivial = class()

SleepTrivial.name = 'sleep trivial'
SleepTrivial.does = 'stonehearth:sleep'
SleepTrivial.status_text = 'sleeping...'
SleepTrivial.args = {}
SleepTrivial.version = 2
SleepTrivial.priority = 1

function SleepTrivial:start_thinking(ai, entity, args)
   local parent = radiant.entities.get_parent(entity)
   if not parent then
      return
   end

   if radiant.entities.get_entity_data(parent, 'stonehearth:bed') then
      -- hey, we're already in bed!
      self._bed = parent
      ai:set_think_output()
   end
end

function SleepTrivial:run(ai, entity, args)
   local sleep_duration, rested_sleepiness = SleepLib.get_sleep_parameters(entity, self._bed)
   ai:execute('stonehearth:run_sleep_effect', { duration_string = sleep_duration })
   radiant.entities.set_attribute(entity, 'sleepiness', rested_sleepiness)
end

function SleepTrivial:stop(ai, entity, args)
   local root_entity = radiant.entities.get_root_entity()
   local bed_location = radiant.entities.get_world_grid_location(self._bed)
   local egress_location = bed_location + Point3.unit_y
   radiant.entities.add_child(root_entity, entity, egress_location)

   self._bed = nil
end

return SleepTrivial
