local ProtectStockpile = class()

local om = require 'radiant.core.om'
local md = require 'radiant.core.md'
local sh = require 'radiant.core.sh'
local check = require 'radiant.core.check'
local ai_mgr = require 'radiant.ai.ai_mgr'

ai_mgr:register_intention('radiant.intentions.protect_stockpile', ProtectStockpile)

function ProtectStockpile:__init(entity, stockpile, radius, priority)
   check:is_entity(entity)
   check:is_entity(stockpile)
   check:is_number(radius)
   check:is_number(priority)

   self._radius = radius
   self._stockpile = stockpile
   self._priority = priority
   self._observer = ai_mgr:add_observer(entity, 'radiant.observers.compute_danger_to', stockpile, radius)
end

function ProtectStockpile:destroy()
   self._observer:destroy()
end

function ProtectStockpile:recommend_activity(entity)
   -- close the distance between ourself and the stockpile if we're too far
   -- away...
   return self._priority, { 'radiant.actions.protect_stockpile', self._stockpile }
end

--[[
md:register_msg_handler('radiant.ai.protect_entity', ProtectStockpile)

ProtectStockpile['radiant.md.create'] = function(self, entity, target, radius, priority)
   check:is_a(entity, Stockpile)
   check:is_a(target, Stockpile)
   check:is_number(radius)
   check:is_number(priority)
   
   self._entity = entity
   self._target = target
   self._radius = radius
   self._priority = priority
   self._sensor = om:create_sensor(entity, radius)
   
   self._target_table = om:create_target_table(entity, 'radiant.protect.' .. tostring(target:get_id()))
   self._target_table:set_name('threats to entity ' .. target:get_id())

   md:send_msg(entity, 'radiant.ai.register_need', self, true)
   md:listen('radiant.events.slow_poll', self)
end

ProtectStockpile['radiant.md.destroy'] = function(self)
   om:destroy_sensor(self._entity, self._sensor)
   md:send_msg(self._entity, 'radiant.ai.register_need', self, false)
   md:unlisten('radiant.events.slow_poll', self)
   om:destroy_target_table(self._entity, self._target_table)
end

-- measure the possible threats to the entity we're protecting.
ProtectStockpile['radiant.events.slow_poll'] = function(self)
   for id in self._sensor:get_contents():items() do   
      local intruder = om:get_entity(id)
      if intruder then 
         if sh:is_hostile(self._target, intruder) then
            local ferocity = om:get_attribute(intruder, 'ferocity')
            if ferocity then
               self._target_table:add_entry(intruder)
                                    :set_value(ferocity)
                                    :set_expire_time(om:now() + 5000)
            end
         end
      end
   end
end

ProtectStockpile['radiant.ai.needs.recommend_activity'] = function (self)
   -- close the distance between ourself and the target if we're too far
   -- away...
   local distance = om:get_distance_between(self._entity, self._target)
   if distance > self._radius then
      return self._priority, md:create_activity('radiant.activities.travel_toward', self._target)
   end   
end
]]