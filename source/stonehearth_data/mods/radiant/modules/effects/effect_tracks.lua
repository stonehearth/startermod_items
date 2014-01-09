local GenericEffect = require 'modules.effects.generic_effect'
local AnimationEffect = require 'modules.effects.animation_effect'
local FrameDataEffect = require 'modules.effects.frame_data_effect'
local TriggerEffect = require 'modules.effects.trigger_effect'
local MusicEffect = require 'modules.effects.music_effect'
local CubemitterEffect = require 'modules.effects.cubemitter_effect'
local LightEffect = require 'modules.effects.light_effect'
local ActivityOverlayEffect = require 'modules.effects.activity_overlay_effect'
local UnitStatusEffect = require 'modules.effects.unit_status_effect'

local EffectTracks = class()

local log = radiant.log.create_logger('effects')

function EffectTracks:__init(mgr, entity, effect_path, effect_name, start_time, trigger_handler, args)
   self._mgr = mgr
   self._name = effect_name
   self._effect_list = entity:add_component('effect_list')
   self._effect = self._effect_list:add_effect(effect_path, start_time)
   self._running = true
   self._entity = entity
   self._effect_path = effect_path

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
   for name, e in pairs(effect.tracks) do
      if e.type == "animation_effect" then
         local animation = self._mgr._animation_root .. '/' .. e.animation
         table.insert(self._effects, AnimationEffect(animation, start_time, e))
      elseif e.type == "trigger_effect" then
         table.insert(self._effects, TriggerEffect(start_time, trigger_handler, e, self._effect, entity))
      elseif e.type == "attack_frame_data" then
         table.insert(self._effects, FrameDataEffect(start_time, trigger_handler, e, self._effect))
      elseif e.type == "sound_effect" then
         table.insert(self._effects, MusicEffect(start_time, trigger_handler, e, self._effect, args))
      elseif e.type == "light" then
         table.insert(self._effects, LightEffect(e))
      elseif e.type == "activity_overlay_effect" then
         table.insert(self._effects, ActivityOverlayEffect(e))
      elseif e.type == "unit_status_effect" then
         table.insert(self._effects, UnitStatusEffect(e))
      else
         log:debug('unknown effect type "%s".  using generic', e.type)
         table.insert(self._effects, GenericEffect(start_time, trigger_handler, e, self._effect))
      end
   end
end

function EffectTracks:get_name()
   return self._name
end

function EffectTracks:cleanup()
   self._on_finished_fn = nil
   self._effect_list:remove_effect(self._effect)
   self._mgr:_remove_effect(self)
end

function EffectTracks:finished()
   return not self._running
end

function EffectTracks:on_finished(fn)
   self._on_finished_fn = fn
end


function EffectTracks:stop()
   if self._running then
      self._running = false
      if self._on_finished_fn then
         self._on_finished_fn()
      end
      radiant.events.trigger(self._entity, 'stonehearth:on_effect_finished', {
         effect = self._effect_path
      })
      self:cleanup()
   end
end

function EffectTracks:update(e)
   -- xxx: this is shitty to the max...
   if self._running then
      local all_finished = true
      for i, effect in ipairs(self._effects) do
         if effect:update(e) then
            all_finished = false
         end
      end
      if all_finished then
         self:stop()
      end
   end
end

return EffectTracks
