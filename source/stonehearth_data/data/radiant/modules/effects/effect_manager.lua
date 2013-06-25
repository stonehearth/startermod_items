local EffectTracks = require 'radiant.modules.effects.effect_tracks'

local EffectManager = class()

function EffectManager:__init(entity)
   self._effects = {}
   self._skip_animations = get_config_option("game.noidle");
   
   self._entity = entity
   
   radiant.events.listen('radiant.events.gameloop', self)
   
   -- xxx: the animation_table should be on the entity?  what if we have no rigs
   -- at all?  or should "render_rig" be renamed something more appropriate?
   local render_rig = entity:get_component('render_rig')
   self.animation_table_name = render_rig:get_animation_table()
   
   local uri = entity:get_resource_uri()
   assert(#self.animation_table_name > 0)
   
   local obj = radiant.resources.load_json(self.animation_table_name)
   self._effects_root = obj.effects_root
   local i
   i = 0
end

function EffectManager:destroy(entity)
   radiant.events.unlisten('radiant.events.gameloop', self)
end

function EffectManager:on_event_loop(now)
   -- radiant.log.info('-----')
   for e, _ in pairs(self._effects) do
      -- radiant.log.info('checking effect %d', now)
      e:update(now) 
      if e:finished() then
         self._effects[e] = nil
      end
   end
   -- radiant.log.info('-----')
end

function EffectManager:start_effect(action, trigger_handler, args)
   return self:start_action_at_time(action, radiant.gamestate.now(), trigger_handler, args)
end

function EffectManager:start_action_at_time(action, when, trigger_handler, args)
   radiant.check.is_string(action) 
   radiant.check.is_number(when)

   if trigger_handler then radiant.check.is_callable(trigger_handler) end
   if args then radiant.check.is_table(args) end

   radiant.log.debug("staring effect %s at %d", action, when)
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
   --md:send_msg(self._entity, 'radiant.animation.on_start', name)      
end

function EffectManager:_remove_effect(e)
   self._effects[e] = nil
end

return EffectManager;