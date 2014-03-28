local CombatFns = require 'ai.actions.combat.combat_fns'
local LookForEnemies = require 'ai.observers.look_for_enemies'
local Entity = _radiant.om.Entity
local log = radiant.log.create_logger('combat')

local Attack = class()

Attack.name = 'basic attack'
Attack.does = 'stonehearth:attack'
Attack.args = {}
Attack.think_output = {}
Attack.version = 2
Attack.priority = 1
Attack.weight = 1

function Attack:__init()
   self._enable_combat = radiant.util.get_config('enable_combat', false)
end

function Attack:start_thinking(ai, entity, args)
   self._ai = ai
   self._entity = entity

   if self._enable_combat then
      radiant.events.listen(radiant, 'stonehearth:very_slow_poll', self, self._find_target)
      --self._look_for_enemies = LookForEnemies(self._entity)
   end
end

function Attack:stop_thinking(ai, entity, args)
   if self._enable_combat then
      radiant.events.unlisten(radiant, 'stonehearth:very_slow_poll', self, self._find_target)
   end
end

function Attack:run(ai, entity, args)
   local target = self._target
   local melee_range = CombatFns.get_melee_range(entity, 'medium_1h_weapon', target)

   while true do
      ai:execute('stonehearth:goto_entity', { entity = target, stop_distance = melee_range })
      ai:execute('stonehearth:attack_melee_adjacent', { target = target })
   end
end

function Attack:_find_target()
   local target = self:_get_target()

   if not target then
      return
   end

   self._target = target
   self._ai:set_think_output()
end

-- stupid implementation until target tables are fixed
function Attack:_get_target()
   local entity = self._entity
   local pop = stonehearth.population:get_population('player_1')
   local citizens = pop:get_citizens()
   local target = citizens[1]

   if target:get_id() == entity:get_id() then
      target = nil
   end

   return target
end

function Attack:_find_target_new()
   local target = radiant.entities.get_target_table_top(self._entity, 'aggro')
   if not target then
      return
   end
   self._target = target
   self._ai.set_think_output()
end

return Attack
