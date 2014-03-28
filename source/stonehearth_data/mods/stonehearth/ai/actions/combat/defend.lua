local CombatFns = require 'ai.actions.combat.combat_fns'
local rng = _radiant.csg.get_default_rng()
local log = radiant.log.create_logger('combat')

local Defend = class()

Defend.name = 'basic defence'
Defend.does = 'stonehearth:defend'
Defend.args = {}
Defend.version = 2
Defend.priority = 1
Defend.weight = 1

function Defend:__init()
   self._weapon_table = CombatFns.get_weapon_table('medium_1h_weapon')
   
   self._defense_types, self._num_defense_types =
      CombatFns.get_action_types(self._weapon_table, 'defense_types')
end

function Defend:start_thinking(ai, entity, args)
   self._ai = ai
   self._entity = entity
   radiant.events.listen(entity, 'stonehearth:hit', self, self._on_battery)
end

function Defend:stop_thinking(ai, entity, args)
   radiant.events.unlisten(entity, 'stonehearth:hit', self, self._on_battery)
end

function Defend:run(ai, entity, args)
   local attacker = self._attacker

   radiant.entities.turn_to_face(entity, attacker)

   ai:execute('stonehearth:run_effect', {
      effect = self._defense_name,
      delay = self._defend_delay
   })
end

function Defend:_on_battery(args)
   local attacker = args.attacker
   local entity = self._entity
   local roll = rng:get_int(1, self._num_defense_types)
   local defense_name = self._defense_types[roll]
   local defense_impact_time = CombatFns.get_impact_time(self._weapon_table, 'defense_types', defense_name)
   local defend_delay = args.impact_time - defense_impact_time

   if defend_delay < 0 then
      -- too slow, cannot defend
      return
   end

   local defend = rng:get_real(0, 1) < 0.75
   defend = false -- CHECKCHECK

   if not defend then
      return
   end

   self._attacker = attacker
   self._defense_name = defense_name
   self._defend_delay = defend_delay

   self:_send_battery_defended(attacker, entity)
   self._ai:set_think_output()
end

function Defend:_send_battery_defended(attacker, target)
   radiant.events.trigger(attacker, 'stonehearth:hit_defended', {
      target = target
   })
end

return Defend
