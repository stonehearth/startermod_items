local GenericEffect = require 'modules.effects.generic_effect'
local AnimationEffect = require 'modules.effects.animation_effect'
local TriggerEffect = require 'modules.effects.trigger_effect'
local SoundEffect = require 'modules.effects.sound_effect'
local LightEffect = require 'modules.effects.light_effect'
local ActivityOverlayEffect = require 'modules.effects.activity_overlay_effect'
local UnitStatusEffect = require 'modules.effects.unit_status_effect'

local EffectTracks = class()

function EffectTracks:__init(mgr, entity, effect_path, effect_name, start_time, trigger_handler, args)
   self._mgr = mgr
   self._name = effect_name
   self._effect_list = entity:add_component('effect_list')
   self._effect = self._effect_list:add_effect(effect_path, start_time)
   self._running = true
   self._entity = entity
   self._effect_path = effect_path
   self._cleanup_on_finish = true
   self._start_time = start_time

   self._log = radiant.log.create_logger('effect')
   self._log:set_prefix(effect_name)
            :set_entity(entity)
   
   if args then
      radiant.check.is_table(args)
      for name, value in pairs(args) do
         radiant.check.is_string(name)
         self._effect:add_param(name, value)
      end
   end
   local effect = radiant.resources.load_json(effect_path)
   if not effect then
      self._log:warning('could not find animation named %s', effect_path)
   end
   radiant.check.verify(effect)

   self._effects = {}
   for name, info in pairs(effect.tracks) do
      self._log:spam('adding effect track "%s" of type "%s"', name, info.type)
      if info.type == "animation_effect" then
         local animation = self._mgr._animation_root .. '/' .. info.animation
         self._effects[name] = AnimationEffect(animation, start_time, info)
      elseif info.type == "sound_effect" then
         self._effects[name] = SoundEffect(start_time, info)
      elseif info.type == "trigger_effect" then
         self._effects[name] = TriggerEffect(start_time, trigger_handler, info, self, entity, args)
      elseif info.type == "light" then
         self._effects[name] = LightEffect(info)
      elseif info.type == "activity_overlay_effect" then
         self._effects[name] = ActivityOverlayEffect(info)
      elseif info.type == "unit_status_effect" then
         self._effects[name] = UnitStatusEffect(info, start_time)
      else
         self._log:debug('using generic effect for "%s"', info.type)
         self._effects[name] = GenericEffect(start_time, trigger_handler, info, self._effect)
      end
   end
end

function EffectTracks:get_name()
   return self._name
end

function EffectTracks:set_cleanup_on_finish(val)
   self._cleanup_on_finish = val
   return self
end

function EffectTracks:is_finished()
   return not self._running
end

function EffectTracks:set_trigger_cb(cb)
   self._trigger_cb = cb
   return self
end

function EffectTracks:set_finished_cb(cb)
   self._finished_cb = cb
   return self
end

function EffectTracks:stop()
   self._log:debug('stopping effect before it was finished: %s', self._name)
   self:_cleanup()
   return self
end

function EffectTracks:_cleanup()
   self._log:debug('cleaning up effect')
   if self._effect_list and self._effect_list:is_valid() then
      self._effect_list:remove_effect(self._effect)
   end
   self._mgr:_remove_effect(self)
   radiant.log.return_logger(self._log)
   self._log = nil
end

function EffectTracks:update(e)
   -- xxx: this is shitty to the max...
   if self._running then
      local all_finished = true
      self._log:debug('update (delta t:%d)', e.now - self._start_time)
      for name, effect in pairs(self._effects) do
         if effect:update(e) then
            all_finished = false
            self._log:spam('  effect "%s" is not finished!', name)
         else
            self._log:spam('  effect "%s" is finished', name)
         end
      end
      if all_finished then
         self:_finish()
      end
   end
end

--- Finish means, "the effect has played all the way through to completion".  Specifically,
-- stopped effects will not trigger the finish event!  Note that events on the server don't
-- loop, so "completion" for an animation means it just ran through once!!
function EffectTracks:_finish()
   if self._running then
      self._log:debug('effect finished!')
      self._running = false
      if self._cleanup_on_finish then
         self:_cleanup()
      end
      if self._finished_cb then
         self._finished_cb()
      end
   end
end

return EffectTracks
