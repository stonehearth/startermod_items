local CombatFns = require 'ai.actions.combat.combat_fns'
local LookForEnemies = require 'ai.observers.look_for_enemies'
local Entity = _radiant.om.Entity
local rng = _radiant.csg.get_default_rng()
local log = radiant.log.create_logger('combat')

local Combat = class()

Combat.name = 'combat'
Combat.does = 'stonehearth:combat'
Combat.args = {}
Combat.version = 2
Combat.priority = 1
Combat.weight = 1

function Combat:__init()
   self._enable_combat = radiant.util.get_config('enable_combat', false)
   self._in_combat = false
   self._engaged = false
   self._assaulted = false
   self._attacking = false
end

function Combat:start_thinking(ai, entity, args)
   self._ai = ai
   self._entity = entity
   self:_subscribe_to_events()
end

function Combat:start(ai, entity, args)
end

function Combat:stop_thinking(ai, entity, args)
   if not self._in_combat then
      self:_unsubscribe_events()
   end
end

function Combat:run(ai, entity, args)
   local facing_entity

   if not self._target then
      local foo = 1
   end

   while true do
      if self._assaulted then
         ai:execute('stonehearth:combat:defend', { attacker = self._attacker, impact_time = self._impact_time })
         self._assaulted = false
      elseif self._engaged then
         facing_entity = self._target or self._attacker
         ai:execute('stonehearth:combat:idle', { facing_entity = facing_entity })
         self._engaged = false
      elseif self._target then
         self._attacking = true
         ai:execute('stonehearth:combat:attack', { target = self._target })
         self._attacking = false
      end

      if not self._target then
         break
      end
   end
end

function Combat:stop(ai, entity, args)
   self:_unsubscribe_events()
   self:_exit_combat()
end

function Combat:destroy(ai, entity, args)
end

function Combat:_on_engaged(args)
   -- do we need a list of engagers?
   self._engaged = true
   self._attacker = args.attacker
   self:_enter_combat()
end

function Combat:_on_assaulted(args)
   -- create a list of attackers
   self._assaulted = true
   self._attacker = args.attacker
   self._impact_time = args.impact_time
   self:_enter_combat()
end

function Combat:_enter_combat()
   if not self._in_combat then
      self._in_combat = true
      self._ai:set_think_output()
   end
end

function Combat:_exit_combat()
   self._in_combat = false
   self._engaged = false
   self._assaulted = false
   self._attacking = false
end

function Combat:_busy()
   return self._engaged or self._assaulted or self._attacking
end

function Combat:_subscribe_to_events()
   radiant.events.listen(self._entity, 'stonehearth:combat:engage', self, self._on_engaged)
   radiant.events.listen(self._entity, 'stonehearth:combat:assault', self, self._on_assaulted)

   if self._enable_combat then
      radiant.events.listen(radiant, 'stonehearth:very_slow_poll', self, self._find_target)
      --self._look_for_enemies = LookForEnemies(self._entity)
   end
end

function Combat:_unsubscribe_events()
   radiant.events.unlisten(self._entity, 'stonehearth:combat:engage', self, self._on_engaged)
   radiant.events.unlisten(self._entity, 'stonehearth:combat:assault', self, self._on_assaulted)

   if self._enable_combat then
      radiant.events.unlisten(radiant, 'stonehearth:very_slow_poll', self, self._find_target)
   end
end

function Combat:_find_target()
   local target = self:_get_target()

   if not target then
      return
   end

   self._target = target
   self:_enter_combat()
end

-- stupid implementation until target tables are fixed
function Combat:_get_target()
   local entity = self._entity
   local pop = stonehearth.population:get_population('player_1')
   local citizens = pop:get_citizens()
   local target = citizens[1]

   if target ~= nil then
      if target:get_id() == entity:get_id() then
         --target = nil
         target = citizens[2]
      end
   end

   return target
end

function Combat:_find_target_new()
   local target = radiant.entities.get_target_table_top(self._entity, 'aggro')
   if not target then
      return
   end
   self._target = target
   self:_enter_combat()
end

return Combat
