local md = require 'radiant.core.md'
local om = require 'radiant.core.om'
local check = require 'radiant.core.check'
local util = require 'radiant.core.util'
local ani_mgr = require 'radiant.core.animation'

-- ------------------------

local RunEffect = class()
function RunEffect:__init(entity, name, trigger_handler, ...)
   check:is_entity(entity)
   check:is_string(name)

   self._entity = entity
   self._name = name
   self._trigger_handler = trigger_handler
   self._args = ...
end

function RunEffect:get_type()
   return 'effect'
end

function RunEffect:run()
   ani_mgr:get_animation(self._entity):start_action(self._name, self._trigger_handler, unpack(self._args))
end

-- ------------------------

local Damage = class()
function Damage:__init(entity, source, amount, type)
   self._entity = entity
   self._source = source
   self._amount = amount
   self._type = type
end

function Damage:get_type()
   return 'damage'
end

function Damage:run()
   local ani = ani_mgr:get_animation(self._entity)
   if self._mitigate_reason then
      ani:start_action('radiant.effects.floating_combat_txt', nil, { damage_text = self._mitigate_reason })
   else
      ani:start_action('radiant.effects.floating_combat_txt', nil, { damage_text = tostring(self._amount) })
      md:send_msg(self._entity, 'radiant.events.on_damage', self._source, self._amount, self._type)
   end
end

function Damage:mitigate(reason)
   self._mitigate_reason = reason
end

-- ------------------------

local AttackData = class()

AttackData.Slots = {
   Paperdoll.MAIN_HAND
}

function AttackData:__init(entity, target)
   check:is_entity(entity)
   check:is_entity(target)

   self._entity = entity
   self._target = target
   self._effects = {}
end

function AttackData:get_attacker()
   return self._entity
end

function AttackData:get_target()
   return self._target
end


function AttackData:resolve()
   for _, effect in ipairs(self._effects) do
      effect:run()      
   end
end

function AttackData:apply_effect_to(entity, name, trigger_handler, ...)
   check:is_entity(entity)
   check:is_string(name)

   table.insert(self._effects, RunEffect(entity, name, trigger_handler, ...))
end

function AttackData:damage_target(amount, class)
   check:is_number(amount)
   check:is_string(class)
   table.insert(self._effects, Damage(self._target, self._entity, amount, class))
end

function AttackData:each()
   local i = 1
   return function()
      local result = self._effects[i]
      i = i + 1
      return result
   end
end

return AttackData
