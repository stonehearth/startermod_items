local GenericEffect = require 'radiant.modules.effects.generic_effect'
local AnimationEffect = require 'radiant.modules.effects.animation_effect'
local FrameDataEffect = require 'radiant.modules.effects.frame_data_effect'
local TriggerEffect = require 'radiant.modules.effects.trigger_effect'
local MusicEffect = require 'radiant.modules.effects.music_effect'

local EffectTracks = class()
function EffectTracks:__init(mgr, entity, effect_path, effect_name, start_time, trigger_handler, args)
   self._mgr = mgr
   self._name = effect_name
   self._effect_list = entity:add_component('effect_list')
   self._effect = self._effect_list:add_effect(effect_path, start_time)
   self._running = true

   if args then
      radiant.check.is_table(args)
      for name, value in pairs(args) do
         radiant.check.is_string(name)
         self._effect:add_param(name, value)
      end
   end
   local effect = radiant.resources.load_json(effect_path)
   if not effect then
      log:warning('could not find animation named %s', effect_path)
   end
   radiant.check.verify(effect)

   self._effects = {}
   for name, e in radiant.resources.pairs(effect.tracks) do
      if e.type == "animation_effect" then
         table.insert(self._effects, AnimationEffect(e.animation, start_time, e))
      elseif e.type == "trigger_effect" then
         table.insert(self._effects, TriggerEffect(start_time, trigger_handler, e, self._effect))
      elseif e.type == "attack_frame_data" then
         table.insert(self._effects, FrameDataEffect(start_time, trigger_handler, e, self._effect))
      elseif e.type == "music_effect" then
         table.insert(self._effects, MusicEffect(start_time, trigger_handler, e, self._effect, args))
      else
         radiant.log.info('unknown effect type "%s".  using generic', e.type)
         table.insert(self._effects, GenericEffect(start_time, trigger_handler, e, self._effect))
      end
   end
end

function EffectTracks:get_name()
   return self._name
end

function EffectTracks:cleanup()
   self._effect_list:remove_effect(self._effect)
   self._mgr:_remove_effect(self)
end

function EffectTracks:finished()
   return not self._running
end

function EffectTracks:stop()
   if self._running then
      self._running = false
      self:cleanup()
   end
end

function EffectTracks:update(now)
   -- xxx: this is shitty to the max...
   if self._running then
      local all_finished = true
      for i, effect in ipairs(self._effects) do
         if effect:update(now) then
            all_finished = false
         end
      end
      if all_finished then
         self:stop()
      end
   end
end

return EffectTracks
