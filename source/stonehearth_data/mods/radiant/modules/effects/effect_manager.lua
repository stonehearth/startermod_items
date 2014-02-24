local EffectTracks = require 'modules.effects.effect_tracks'
local EffectManager = class()

local log = radiant.log.create_logger('effects')

function EffectManager:__init(entity)
   self._effects = {}

   self._entity = entity
   radiant.events.listen(radiant.events, 'stonehearth:gameloop', self, self.on_event_loop)

   self.animation_table_name = radiant.entities.get_animation_table(self._entity)
   if self.animation_table_name and self.animation_table_name ~= "" then
      local obj = radiant.resources.load_json(self.animation_table_name)
      self._effects_root = obj.effects_root
      self._animation_root = obj.animation_root
      
      if obj.postures then
        self._posture_component = self._entity:add_component('stonehearth:posture')
        self._postures = radiant.resources.load_json(obj.postures)
      end
   end
end

function EffectManager:destroy(entity)
   radiant.events.unlisten(radiant.events, 'stonehearth:gameloop', self, self.on_event_loop)
end

function EffectManager:on_event_loop(e)
   for effect, _ in pairs(self._effects) do
      effect:update(e)
      if effect:finished() then
         self._effects[effect] = nil
      end
   end
end

function EffectManager:start_effect(action, trigger_handler, args)
   local resolved_action = self:get_resolved_action(action)
   return self:start_action_at_time(resolved_action, radiant.gamestate.now(), trigger_handler, args)
end

function EffectManager:get_resolved_action(action)  
  if self._postures then 
    local posture = self._posture_component:get_posture()
    -- if a posture is set and that posture is in the override table
    if self._postures[posture] then
      -- if the set posture overrides this action
      if self._postures[posture][action] then
        -- return the override action
        local resolved_action = self._postures[posture][action]
        log:debug('effect %s resolved to %s [%s posture:%s]', action, resolved_action, self._entity, posture)
        return resolved_action
      end
    end
  end

  return action
end

function EffectManager:start_action_at_time(action, when, trigger_handler, args)
   radiant.check.is_string(action)
   radiant.check.is_number(when)
   --TODO: there is no such function as radiant.check.is_callable
   --if trigger_handler then radiant.check.is_callable(trigger_handler) end
   if args then radiant.check.is_table(args) end

   log:debug("staring effect %s at %d", action, when)
   return self:_add_effect(action, when, trigger_handler, args);
end

function EffectManager:get_effect_track(effect_name, track_name)
   local effect = self:_get_effect_info(effect_name)
   return effect.tracks[track_name]
end

function EffectManager:get_effect_duration(effect_name)
   local end_time = 0
   local effect = self:_get_effect_info(effect_name)
   for name, info in pairs(effect.tracks) do
      end_time = math.max(end_time, get_end_time(info))
   end
   return end_time
end

function EffectManager:_get_effect_path(effect_name)
   --If effect_name starts with /, return effect name, otherwise return root+name
   local first_char = effect_name:sub(1,1)
   if first_char == "/" then
      return effect_name
   end
   return self._effects_root .. '/' .. effect_name .. '.json'
end

function EffectManager:_get_effect_info(effect_name)
   local effect_path  = self:_get_effect_path(effect_name)
   return radiant.resources.load_json(effect_path)
end

function EffectManager:_add_effect(name, when, trigger_handler, args)
   -- xxx: make sure we're not already going one with this name!
   local effect_path = self:_get_effect_path(name)
   local effect = EffectTracks(self,
                               self._entity,
                               effect_path,
                               name,
                               when,
                               trigger_handler,
                               args)
   self._effects[effect] = true
   return effect
   --md:send_msg(self._entity, 'radiant:animation:on_start', name)
end

function EffectManager:_remove_effect(e)
   self._effects[e] = nil
end

return EffectManager;