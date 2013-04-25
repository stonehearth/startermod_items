local LifeAndDeath = class()

local md = require 'radiant.core.md'
local om = require 'radiant.core.om'
local log = require 'radiant.core.log'
local ai_mgr = require 'radiant.ai.ai_mgr'
local ani_mgr = require 'radiant.core.animation'
local check = require 'radiant.core.check'

md:register_msg('radiant.events.on_damage')

ai_mgr:register_intention('radiant.intentions.life_and_death', LifeAndDeath)

function LifeAndDeath:__init(entity)
   self._entity = entity
   self._aggro_table = om:create_target_table(entity, 'radiant.aggro')

   check:is_entity(entity)  
   md:listen(entity, 'radiant.events.on_damage', self)   
end

function LifeAndDeath:destroy(entity)
   om:destroy_target_table(entity, self._aggro_table)
   md:unlisten(entity, 'radiant.events.on_damage', self)   
end

function LifeAndDeath:recommend_activity(entity)
   if self._death_reason then
      log:warning('recommendeding that we DIEEEEEEEEEEEEEEEE')
      return 999999999, { 'radiant.actions.die', self._death_reason }
   end
end

LifeAndDeath['radiant.events.on_damage'] = function(self, source, amount, type)
   check:is_number(amount)
   check:is_string(type)

   log:warning('health is %d before taking %d damage...', om:get_attribute(self._entity, 'health'), amount)
   local health = om:update_attribute(self._entity, 'health', -amount)
   log:warning('health is now %d...', health)
   
   if health <= 0 then
      self._death_amount = amount
      self._death_reason = type
   end

   if source then
      self._aggro_table:add_entry(source)
                           :set_value(amount)
   end
end
