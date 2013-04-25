local ComptueProximityAggro = class()

local om = require 'radiant.core.om'
local md = require 'radiant.core.md'
local sh = require 'radiant.core.sh'
local util = require 'radiant.core.util'
local check = require 'radiant.core.check'
local ai_mgr = require 'radiant.ai.ai_mgr'

ai_mgr:register_observer('radiant.observers.compute_proximity_aggro', ComptueProximityAggro)

function ComptueProximityAggro:__init(entity)
   self._entity = entity
   self._target_table = om:create_target_table(entity, 'radiant.aggro')
   md:listen('radiant.events.slow_poll', self)
end

function ComptueProximityAggro:destroy()
   om:destroy_target_table(self._entity, self._target_table)
   md:unlisten('radiant.events.slow_poll', self)
end

-- measure the possible threats to the entity we're protecting.
ComptueProximityAggro['radiant.events.slow_poll'] = function(self)
   if not util:is_entity(self._entity) then
      return
   end

   local sensor = om:get_sight_sensor(self._entity)
   for intruder in om:get_hostile_entities(self._entity, sensor) do
      local ferocity = om:get_attribute(intruder, 'ferocity')

      -- scale the ferocity by the distance to the target
      local distance = om:get_distance_between(self._entity, intruder)
      local scale = math.max(15 - distance, 0)
      ferocity = ferocity * scale

      if ferocity > 0 then
         self._target_table:add_entry(intruder)
                              :set_value(ferocity)
                              :set_expire_time(om:now() + 5000)
      end
   end
end
