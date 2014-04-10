local CombatFns = require 'ai.actions.combat.combat_fns'
local Entity = _radiant.om.Entity
local rng = _radiant.csg.get_default_rng()
local log = radiant.log.create_logger('combat')

local DefendMelee = class()

DefendMelee.name = 'melee defense'
DefendMelee.does = 'stonehearth:combat:defend'
DefendMelee.args = {
   attack_method = 'string',
   attacker = Entity,
   impact_time = 'number'
}
DefendMelee.version = 2
DefendMelee.priority = 1
DefendMelee.weight = 1

function DefendMelee:__init(entity)
   self._weapon_table = CombatFns.get_weapon_table('medium_1h_weapon')
   
   self._defense_types, self._num_defense_types =
      CombatFns.get_action_types(self._weapon_table, 'defense_types')
end

function DefendMelee:start_thinking(ai, entity, args)
   if args.attack_method ~= 'melee' then
      return
   end

   local attacker = args.attacker
   local roll = rng:get_int(1, self._num_defense_types)
   local defense_name = self._defense_types[roll]
   local defense_impact_time = CombatFns.get_impact_time(self._weapon_table, 'defense_types', defense_name)
   local defend_delay = args.impact_time - defense_impact_time

   if defend_delay < 0 then
      -- too slow, cannot defend
      return
   end

   local defend = rng:get_real(0, 1) < 0.75

   if not defend then
      --return   -- CHECKCHECK
   end

   self._attacker = attacker
   self._defense_name = defense_name
   self._defend_delay = defend_delay

   self:_send_assault_defended(attacker, entity)
   ai:set_think_output()
end

function DefendMelee:stop_thinking(ai, entity, args)
end

function DefendMelee:run(ai, entity, args)
   local attacker = self._attacker

   radiant.entities.turn_to_face(entity, attacker)

   ai:execute('stonehearth:run_effect', {
      effect = self._defense_name,
      delay = self._defend_delay
   })
end

function DefendMelee:_send_assault_defended(attacker, target)
   radiant.events.trigger(attacker, 'stonehearth:combat:assault_defended', {
      target = target
   })
end

return DefendMelee
