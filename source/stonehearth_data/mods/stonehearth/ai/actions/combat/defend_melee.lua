local CombatFns = require 'ai.actions.combat.combat_fns'
local Entity = _radiant.om.Entity
local rng = _radiant.csg.get_default_rng()
local log = radiant.log.create_logger('combat')

local DefendMelee = class()

DefendMelee.name = 'melee defense'
DefendMelee.does = 'stonehearth:combat:defend'
DefendMelee.args = {
   attack_method = 'string',
   attack_action = 'table',   -- actually, a class, but we don't know exactly what kind!
   attacker = Entity,
   impact_time = 'number',
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
   local defend_offset = CombatFns.get_impact_time(self._weapon_table, 'defense_types', defense_name)
   local defend_time = radiant.gamestate.now() + defend_offset

   if defend_time > args.impact_time then
      -- too slow, cannot defend
      return
   end

   local defend = rng:get_real(0, 1) < 0.75
   if not defend then
      --return   -- CHECKCHECK
   end

   self._attacker = attacker
   self._defend_offset = defend_offset
   self._defense_name = defense_name
   ai:set_think_output()
end

function DefendMelee:stop_thinking(ai, entity, args)
end

function DefendMelee:run(ai, entity, args)
   -- are we still ok?
   local time_to_impact = args.impact_time - radiant.gamestate.now()
   local defend_delay = time_to_impact - self._defend_offset
   
   if defend_delay < 0 then
      -- nope!
      return
   end

   local attack_action = args.attack_action
   local defense_info = args.attack_action:begin_defense(args)
   if defense_info then
      radiant.entities.turn_to_face(entity, self._attacker)
      ai:execute('stonehearth:run_effect', {
         effect = self._defense_name,
         delay = defend_delay,
      })
      attack_action:end_defense(args)
   end
end

return DefendMelee
