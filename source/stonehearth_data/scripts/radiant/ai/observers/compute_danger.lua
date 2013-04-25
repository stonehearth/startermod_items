local ComputeDanger = class()

local om = require 'radiant.core.om'
local md = require 'radiant.core.md'
local sh = require 'radiant.core.sh'
local util = require 'radiant.core.util'
local check = require 'radiant.core.check'
local ai_mgr = require 'radiant.ai.ai_mgr'

ai_mgr:register_observer('radiant.observers.compute_danger', ComputeDanger)

function ComputeDanger:__init(entity)
   check:is_entity(entity)
   
   self._entity = entity
   self._sensor = om:get_sight_sensor(entity)
   self._target_table = om:create_target_table(entity, 'radiant.danger')
   md:listen('radiant.events.slow_poll', self)
end

function ComputeDanger:destroy()
   om:destroy_target_table(self._entity, self._target_table)
   md:unlisten('radiant.events.slow_poll', self)
end

-- measure the possible threats to the entity we're protecting.
ComputeDanger['radiant.events.slow_poll'] = function(self)
   for intruder in om:get_hostile_entities(self._entity, self._sensor) do
      local ferocity = om:get_attribute(intruder, 'ferocity')
      if ferocity > 0 then
         self._target_table:add_entry(intruder)
                              :set_value(ferocity)
                              :set_expire_time(om:now() + 5000)
      end
   end
end
