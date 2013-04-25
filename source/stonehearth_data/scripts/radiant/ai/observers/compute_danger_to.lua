local ComputeDangerTo = class()

local om = require 'radiant.core.om'
local md = require 'radiant.core.md'
local sh = require 'radiant.core.sh'
local check = require 'radiant.core.check'
local ai_mgr = require 'radiant.ai.ai_mgr'

ai_mgr:register_observer('radiant.observers.compute_danger_to', ComputeDangerTo)

function ComputeDangerTo:__init(entity, target, radius)
   check:is_entity(entity)
   check:is_number(radius)
   
   self._entity = entity
   self._sensor = om:create_sensor(entity, 'danger monitor for ' .. tostring(target),  radius)
   
   -- xxx: create a dummy entity and attach the target table to that instead
   -- of making up this weird name (?)
   self._target_table = om:create_target_table(entity, 'radiant.danger_to')
   self._target_table:set_name('threats to entity ' .. target:get_id())

   md:listen('radiant.events.slow_poll', self)
end

function ComputeDangerTo:destroy()
   om:destroy_sensor(self._entity, self._sensor)
   om:destroy_target_table(self._entity, self._target_table)
   md:unlisten('radiant.events.slow_poll', self)
end

-- measure the possible threats to the entity we're protecting.
ComputeDangerTo['radiant.events.slow_poll'] = function(self)
   for intruder in om:get_hostile_entities(self._entity, self._sensor) do
      local ferocity = om:get_attribute(intruder, 'ferocity')
      if ferocity > 0 then
         self._target_table:add_entry(intruder)
                              :set_value(ferocity)
                              :set_expire_time(om:now() + 5000)
      end
   end
end
