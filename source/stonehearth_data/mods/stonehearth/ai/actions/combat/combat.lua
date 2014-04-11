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
   self._ticks_per_second = stonehearth.calendar:get_constants().ticks_per_second
   self._enable_combat = radiant.util.get_config('enable_combat', false)
   self._in_combat = false
   self._engaged = false
   self._assaulted = false
   self._attacking = false

   -- combat timings are all computed in milliseconds (at game speed 1) to synchronize with animations
   self._attack_global_cooldown = radiant.util.get_config('attack_global_cooldown', 1000)
   self._attack_cooldown_end = radiant.gamestate.now()

end

function Combat:_create_action_frames()
   -- self._idle_frame   = self._ai:spawn('stonehearth:combat:idle')
   -- xxx: move idle over to 'stonehearth:combat' !!
   self._attack_frame = self._ai:spawn('stonehearth:combat:attack')
   self._defend_frame = self._ai:spawn('stonehearth:combat:defend')
end

function Combat:start_thinking(ai, entity, args)
   self._ai = ai
   self._entity = entity
   self:_subscribe_to_events()
   self:_create_action_frames()
end

function Combat:run(ai, entity, args)
   self._running = true
   if self._attack_frame:get_state() == 'ready' then
      assert(not self._defend_think_output)
      assert(self._attack_think_output)
      self._attack_frame:run()
      self._attack_frame:stop()
      self._attack_think_output = nil
   end
   if self._defend_frame:get_state() == 'ready' then
      assert(not self._attack_think_output)
      assert(self._defend_think_output)
      self._defend_frame:run()
      self._attack_frame:stop()
      self._defend_think_output = nil
   end
   assert(self._idle_target)
      --self._idle_frame:run({ facing_entity = self._idle_target })
      --self._idle_frame:stop()
end

--[[
function Combat:run_preserved_for_reference(ai, entity, args)
   local executed_action, facing_entity

   while true do
      executed_action = false

      if self._assaulted then
         ai:execute('stonehearth:combat:defend', {
            attack_method = self._attack_method,
            attacker = self._attacker,
            impact_time = self._impact_time
         })
         self._assaulted = false
         executed_action = true

      elseif self._engaged then
         facing_entity = self._target or self._attacker
         ai:execute('stonehearth:combat:idle', {
            facing_entity = facing_entity
         })
         self._engaged = false
         executed_action = true

      elseif self._target then
         if not self:_in_attack_cooldown() then
            self._attacking = true
            ai:execute('stonehearth:combat:attack', {
               target = self._target
            })
            self._attacking = false
            executed_action = true
            self:_start_attack_cooldown()
         end
      end

      if not self._target then
         break
      end

      if not executed_action then
         -- yield to allow other coroutines to run
         --self:_suspend(50)
         facing_entity = self._target or self._attacker
         ai:execute('stonehearth:combat:idle', {
            facing_entity = facing_entity
         })
      end
   end
end
]]

function Combat:stop(ai, entity, args)
   self._running = false
   self:_unsubscribe_events()
   self:_exit_combat()
end

function Combat:destroy(ai, entity, args)
end

function Combat:_in_attack_cooldown()
   local now = self:_get_game_time()
   return now < self._attack_cooldown_end
end

function Combat:_start_attack_cooldown()
   self._attack_cooldown_end = self:_get_game_time() + self._attack_global_cooldown
end

function Combat:_get_game_time()
   return radiant.gamestate.now()
end

function Combat:begin_engage(attack_args)
   if not self._idle_target then
      self._idle_target = attack_args.attacker
   end
   self:_enter_combat()
end

function Combat:begin_assult(attack_args)
   if self._attack_think_output then
      -- busy!
      return
   end
   if self._defend_think_output then
      -- busy!
      return
   end

   -- ok to defend!  give it a shot
   self._defend_think_output = self._defend_frame:start_thinking(attack_args)
   if not self._defend_think_output then
      self._defend_frame:stop_thinking()
      return
   end

   -- go for it!
   if self._running then
      --self._idle_frame:stop()
   else
      self:_enter_combat()
   end
end

function Combat:end_assult(attack_args)
   return nil
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
   CombatFns.register_combat_action(self._entity, self)
   if self._enable_combat then
      radiant.events.listen(radiant, 'stonehearth:very_slow_poll', self, self._find_target)
      --self._look_for_enemies = LookForEnemies(self._entity)
   end
end

function Combat:_unsubscribe_events()
   CombatFns.register_combat_action(self._entity, nil)

   if self._enable_combat then
      radiant.events.unlisten(radiant, 'stonehearth:very_slow_poll', self, self._find_target)
   end
end

-- duration is in milliseconds (at game speed 1)
-- minimum resolution of about 50 ms
function Combat:_suspend(duration)
   local ai = self._ai
   local game_seconds = duration / self._ticks_per_second

   stonehearth.calendar:set_timer(game_seconds,
      function ()
         ai:resume()
      end
   )

   ai:suspend()
end

--- xxx: called from start_thinking()?
function Combat:_find_target()
   -- xxx: target = get_target_table_top()
   if not self._attack_think_output then
      local target = self:_get_target()
      if not target then
         return
      end

      self._idle_target = target
      self._attack_think_output = self._attack_frame:start_thinking({ target = target })
      if self._attack_think_output then
         self:_enter_combat()
      end
   end
end

-- stupid implementation until target tables are fixed
function Combat:_get_target()
   local entity = self._entity
   local pop = stonehearth.population:get_population('player_1')
   local citizens = pop:get_citizens()
   local target = citizens[1]

   if target ~= nil then
      if target:get_id() == entity:get_id() then
         target = nil
         --target = citizens[2]
      end
   end

   return target
end

-----

function Combat:_find_target_new()
   local target = radiant.entities.get_target_table_top(self._entity, 'aggro')
   if not target then
      return
   end
   self:_enter_combat()
end

return Combat
