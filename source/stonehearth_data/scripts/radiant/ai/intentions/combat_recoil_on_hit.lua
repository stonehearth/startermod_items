local CombatRecoilOnHit = class()

local md = require 'radiant.core.md'
local om = require 'radiant.core.om'
local util = require 'radiant.core.util'
local check = require 'radiant.core.check'
local ai_mgr = require 'radiant.ai.ai_mgr'
local ani_mgr = require 'radiant.core.animation'
local dkjson = require 'dkjson'

ai_mgr:register_intention('radiant.intentions.combat_recoil_on_hit', CombatRecoilOnHit)

function CombatRecoilOnHit:__init(entity, ai)
   self._entity = entity
   md:listen(entity, 'radiant.events.on_damage', self)
   self._timers = {}
end

function CombatRecoilOnHit:destroy()
   md:unlisten(self._entity, 'radiant.events.on_damage', self)
end

CombatRecoilOnHit['radiant.events.on_damage'] = function(self, source, amount, type)
   check:is_number(amount)
   check:is_string(type)

   local ani = ani_mgr:get_animation(self._entity)
   self._recoil_action = 'combat/xxx_recoil_on_physical'
   self._end_recoil = env:now() + ani:get_effect_duration(self._recoil_action)
end

function CombatRecoilOnHit:recommend_activity(entity)
   if self._end_recoil then
      local now = env:now()
      if now < self._end_recoil then
         return 500000, { 'radiant.actions.perform', self._recoil_action }
      end
      self._end_recoil = nil
      self._recoil_action = nil
   end
end
