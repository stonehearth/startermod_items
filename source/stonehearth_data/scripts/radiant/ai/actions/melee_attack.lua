local MeleeAttack = class()

local om = require 'radiant.core.om'
local md = require 'radiant.core.md'
local log = require 'radiant.core.log'
local util = require 'radiant.core.util'
local check = require 'radiant.core.check'
local ai_mgr = require 'radiant.ai.ai_mgr'

ai_mgr:register_action('radiant.actions.melee_attack', MeleeAttack)

function MeleeAttack:run(ai, entity, target, script_info)
   self._entity = entity
   self._target = target

   local range = self:_wield_weapon()
   self:_run_to_range(ai, range)

   md:listen(self._entity, 'radiant.combat.on_attack', self)
   ai:execute('radiant.actions.perform_attack_script', target, script_info.effect)
end

function MeleeAttack:stop()
   md:unlisten(self._entity, 'radiant.combat.on_attack', self)
end

MeleeAttack['radiant.combat.on_attack'] = function(self, segment, which, effects)
   assert(which == 'start' or which == 'end')

   if which == 'start' and segment.phase == 'execute' then
      -- xxx: check range here...
      effects:damage_target(math.random(3, 8), 'physical')
   end
end

function MeleeAttack:_wield_weapon()
   local weapon, weapon_info
   weapon = om:get_equipped(self._entity, Paperdoll.MAIN_HAND)
   if weapon then
      weapon_info = om:get_component(weapon, 'weapon_info')
   else
      weapon = om:get_equipped(self._entity, Paperdoll.WEAPON_SCABBARD)
      if weapon then
         weapon_info = om:get_component(weapon, 'weapon_info')
         if weapon_info then
            om:equip(self._entity, Paperdoll.MAIN_HAND, weapon)
         end
      end
   end
   return weapon_info:get_range()
end

function MeleeAttack:_run_to_range(ai, range)
   local distance = om:get_distance_between(self._entity, self._target)
   if distance > range then
      ai:execute('radiant.actions.goto_entity', self._target, 'run', range)
   end
end
