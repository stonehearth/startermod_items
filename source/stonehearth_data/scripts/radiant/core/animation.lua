local dkjson = require ("dkjson")

local AnimationMgr = class()
local AnimationMgrMgr = class()
local md = require 'radiant.core.md'
local om = require 'radiant.core.om'
local log = require 'radiant.core.log'
local util = require 'radiant.core.util'
local check = require 'radiant.core.check'

md:register_msg('radiant.animation.on_trigger')
md:register_msg('radiant.effect.frame_data.on_segment')

function frame_count_to_time(duration)
   return math.floor(1000 * duration / 30)
end

function get_start_time(info)
   if info.start_time then
      return info.start_time
   end
   if info.start_frame then
      frame_count_to_time(info.start_frame)
   end
   return 0
end

function get_end_time(info)
   if info.end_time then
      return info.end_time
   end
   if info.end_frame then
      frame_count_to_time(info.end_frame)
   end
   if info.loop == 'true' then
      return 0
   end
   if info.duration then
      if info.start_time then
         return info.start_time + info.duration
      end
      if info.start_frame then
         return frame_count_to_time(info.end_frame)(info.start_frame + info.duration)
      end      
   end
   if info.type == 'animation_effect' then
      local animation = native:lookup_resource(info.animation)
      assert(animation)
      return animation and animation:get_duration() or 0
   end
   if info.type == 'attack_frame_data' then
      local duration = 0
      for _, segment in ipairs(info.segments) do
         duration = duration + segment.duration
      end
      return frame_count_to_time(duration)
   end
   assert(false)
   return 0
end


local AnimationEffect = class()
function AnimationEffect:__init(start_time, info)  
   self._loop = info.loop
   self._start_time = start_time + get_start_time(info)

   self._animation = native:lookup_resource(info.animation)
   if not self._animation then
      log:warning('could not lookup animation resource %s', info.animation)
   end
   check:is_a(self._animation, AnimationResource)

   self._end_time = 0
   if not self._loop then 
      self._end_time = self._start_time + self._animation:get_duration()
   end
end

function AnimationEffect:update(now)
   return self._end_time == 0 or self._end_time >= now
end

local TriggerEffect = class()
function TriggerEffect:__init(start_time, handler, effect_data, effect)
   self._effect = effect
   self._info = effect_data.info
   self._trigger_time = start_time + get_start_time(effect_data)
   self._handler = handler
end

function TriggerEffect:update(now)
   if self._trigger_time and self._trigger_time <= now then
      md:send_msg(self._handler, "radiant.animation.on_trigger", self._info, self._effect)
      self._trigger_time = nil
   end
   return self._trigger_time ~= nil
end

local FrameDataEffect = class()
function FrameDataEffect:__init(start_time, handler, effect_data, effect)
   self._effect = effect
   self._start_time = start_time
   self._segments = effect_data.segments
   self._segment_index = 0
   self._handler = handler
end

function FrameDataEffect:update(now)
   local segment
   if self._segment_index == 0 then
      self._segment_index = 1
      segment = self._segments[1]
      if self._handler then
         md:send_msg(self._handler, 'radiant.effect.frame_data.on_segment', 'start', segment)
      end
   else
      segment = self._segments[self._segment_index]
   end
  
   while segment and self._start_time + frame_count_to_time(segment.duration) <= now do
      if self._handler then
         md:send_msg(self._handler, 'radiant.effect.frame_data.on_segment', 'end', segment)
      end
      
      self._segment_index = self._segment_index + 1
      segment = self._segments[self._segment_index]
      if segment and self._handler then
         md:send_msg(self._handler, 'radiant.effect.frame_data.on_segment', 'start', segment)
      end
   end
   return segment ~= nil
end

local GenericEffect = class()
function GenericEffect:__init(start_time, handler, effect_data, effect)
   self._end_time = start_time + get_end_time(effect_data)
   if not self._end_time then
      self._end_time = 0
   end
end

function GenericEffect:update(now)  
   return now < self._end_time
end

local EffectProxy = class()
function EffectProxy:__init(mgr, entity, name, start_time, translated, trigger_handler, args) 
   self._mgr = mgr
   self._name = name
   self._effect_list = om:add_component(entity, 'effect_list')
   self._effect = self._effect_list:add_effect(translated, start_time)
   self._running = true
   
   if args then
      check:is_table(args)
      for name, value in pairs(args) do
         check:is_string(name)
         self._effect:add_param(name, value)
      end
   end
   local res = native:lookup_resource(translated)
   if not res then
      log:warning('could not find animation named %s', translated)
   end
   check:verify(res)
   local effect = dkjson.decode(res:get_json())
   assert(effect)
   
   self._effects = {}
   for name, e in pairs(effect.tracks) do
      if e.type == "animation_effect" then    
         table.insert(self._effects, AnimationEffect(start_time, e))
      elseif e.type == "trigger_effect" then
         table.insert(self._effects, TriggerEffect(start_time, trigger_handler, e, self._effect))
      elseif e.type == "attack_frame_data" then
         table.insert(self._effects, FrameDataEffect(start_time, trigger_handler, e, self._effect))
      else
         log:info('unknown effect type "%s".  using generic', e.type)
         table.insert(self._effects, GenericEffect(start_time, trigger_handler, e, self._effect))
      end
   end
end

function EffectProxy:get_name()
   return self._name
end

function EffectProxy:cleanup()
   self._effect_list:remove_effect(self._effect)
   self._mgr:_remove_effect(self)
end

function EffectProxy:finished()
   return not self._running
end

function EffectProxy:stop()
   if self._running then
      self._running = false
      self:cleanup()
   end
end

function EffectProxy:update(now)
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

function AnimationMgrMgr:__init()
   self._entities = {}
   self._animations = {}
end

function AnimationMgrMgr:get_animation(entity, resource)
   check:is_entity(entity)
   
   local id = entity:get_id()
   self._entities[id] = entity
   if not self._animations[id] then
      self._animations[id] = AnimationMgr(entity, resource)
      om:on_destroy_entity(entity, function()
         self._animations[id]:destroy(entity)
         self._animations[id] = nil
      end)
   end
   return self._animations[id]
end

function AnimationMgr:__init(entity, resource)
   self._effects = {}
   self._skip_animations = get_config_option("game.noidle");
   
   self._entity = entity
   
   md:listen('radiant.events.gameloop', self)
   
   local animation_table_name = resource:get('animation_table')
   if animation_table_name then
      local table = native:lookup_resource(animation_table_name)
      if table then
         self._animation_table = table:get('sequences')
      end
   end
   if not self._animation_table then
      log:warning("missing animation table")   
   end
end

function AnimationMgr:destroy(entity)
   md:unlisten('radiant.events.gameloop', self)
end

AnimationMgr['radiant.events.gameloop'] = function(self, now)
   for e, _ in pairs(self._effects) do
      e:update(now) 
      if e:finished() then
         self._effects[e] = nil
      end
   end
end

function AnimationMgr:start_action(action, trigger_handler, args)
   return self:start_action_at_time(action, om:now(), trigger_handler, args)
end

function AnimationMgr:start_action_at_time(action, when, trigger_handler, args)
   check:is_string(action) 
   check:is_number(when)

   if trigger_handler then check:is_callable(trigger_handler) end
   if args then check:is_table(args) end

   log:debug("staring effect %s at %d", action, when)
   return self:_add_effect(action, when, trigger_handler, args);
end

function AnimationMgr:get_effect_track(effect_name, track_name)
   local effect = self:_get_effect_info(effect_name)
   return effect.tracks[track_name]
end

function AnimationMgr:get_effect_duration(effect_name)
   local end_time = 0
   local effect = self:_get_effect_info(effect_name)
   for name, info in pairs(effect.tracks) do
      end_time = math.max(end_time, get_end_time(info))
   end
   return end_time
end

function AnimationMgr:_get_effect_info(effect_name)
   local translated = self:_lookup_effect_name(effect_name)
   local res = native:lookup_resource(translated)
   check:verify(res)
   
   return dkjson.decode(res:get_json())
end

function AnimationMgr:_add_effect(name, when, trigger_handler, args)
   local translated = self:_lookup_effect_name(name)
   assert(not self._effects[translated]);

   local effect = EffectProxy(self, self._entity, name, when, translated, trigger_handler, args)
   self._effects[effect] = true
   return effect
   --md:send_msg(self._entity, 'radiant.animation.on_start', name)      
end

function AnimationMgr:_remove_effect(e)
   self._effects[e] = nil
end

function AnimationMgr:_lookup_effect_name(name)
   local sequence = self._animation_table:get(name)
   if not sequence then
      return name
   end
   
   if util:is_a(sequence, ArrayResource) then
      return sequence:get(0)
   end
   
   check:is_string(sequence)
   return sequence
end

return AnimationMgrMgr()
