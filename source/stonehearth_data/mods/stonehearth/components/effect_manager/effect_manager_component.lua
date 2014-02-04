
local EffectManagerComponent = class()

function EffectManagerComponent:__init(entity, data_binding)
   self._data_binding = data_binding
   self._entity = entity
   self._effects_map = {}
   self._running_effects_map = {}
   self._data_binding:update(self._effects_map)
end


function EffectManagerComponent:extend(json)
   if json and json.effects then
      for key, effect in pairs(json.effects) do
         self:add_effect(key, effect)
      end
   end
end

function EffectManagerComponent:add_effect(name, effect)
   self._effects_map[name] = effect
   self._data_binding:mark_changed()
end

function EffectManagerComponent:show_effect(name)
   local effect = self._effects_map[name]

   if effect then
      self._running_effects_map[name] = radiant.effects.run_effect(self._entity, effect)
   end
end

function EffectManagerComponent:hide_effect(name)
   local effect = self._running_effects_map[name]

   if effect then
      effect:stop()
      self._running_effects_map[name] = nil
   end
end

return EffectManagerComponent
