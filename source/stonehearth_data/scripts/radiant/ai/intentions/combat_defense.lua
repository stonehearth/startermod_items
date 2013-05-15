local CombatDefense = class()

local md = require 'radiant.core.md'
local om = require 'radiant.core.om'
local log = require 'radiant.core.log'
local util = require 'radiant.core.util'
local check = require 'radiant.core.check'
local ai_mgr = require 'radiant.ai.ai_mgr'
local ani_mgr = require 'radiant.core.animation'
local dkjson = require 'dkjson'

ai_mgr:register_intention('radiant.intentions.combat_defense', CombatDefense)

function CombatDefense:__init(entity, ai)
   self._ai = ai
   self._entity = entity
   md:listen(entity, 'radiant.combat.on_defend_start', self)
   self._timers = {}
end

function CombatDefense:destroy()
   md:unlisten(self._entity, 'radiant.combat.on_defend_start', self)
end

CombatDefense['radiant.combat.on_defend_start'] = function(self, attacker, effect_name)
   check:is_entity(attacker)
   check:is_string(effect_name)

   local now = env:now()
   local frame_data = ani_mgr:get_animation(attacker):get_effect_track(effect_name, "frame_data")

   if frame_data then
      local when = now
      local start_time
      for _, segment in ipairs(frame_data.segments) do
         if segment.phase == 'execute' then
            if not start_time then
               start_time = when
            end
         end
         when = when + om:frame_count_to_time(segment.duration)
      end
      -- xxx: debugging
      --start_time = now
      --end_time = when
      -- xxx: end debugging

      -- pick an ability we can use
      local abilities = om:filter_combat_abilities(self._entity, function (ability)
            return ability:has_tag('defend') and ability:get_cooldown_expire_time() < start_time
         end)

      -- give them all a chance to defend... for now, just do a random one...
      local ability = abilities[math.random(1, #abilities)]
      local roll = math.random(1, 1000)
      local success = roll > 700 
      log:info('=========== defend roll: %d  success: %s  now: %d  entity: %d =============', roll, success and "yes!" or "no", om:now(), self._entity:get_id())
      if not success then
         return
      end
      
      if ability then
         -- in practice, we need to make a list of all these to defend against
         -- multiple attackers...

         self._attacker = attacker
         self._action = ability:get_script_name()
         self._script_info = dkjson.decode(ability:get_json())

         local ani = ani_mgr:get_animation(self._entity)
         local defend_frame_data = ani:get_effect_track(self._script_info.effect, "frame_data")
         local startup = om:frame_count_to_time(defend_frame_data.segments[1].duration)
         self._start_time = start_time - startup
         self._end_time = self._start_time + ani:get_effect_duration(self._script_info.effect)

         ability:set_cooldown_expire_time(self._start_time + ability:get_cooldown())
      end
   end
end

function CombatDefense:recommend_activity(entity)
   local now = env:now()
   if self._action then
      if self._start_time then
         if now < self._end_time and util:is_entity(self._attacker) then
            return 1000000, { self._action, self._attacker, self._script_info, self._start_time, self._end_time }
         end
         log:info('ditching %s defense at %d.', self._action, now)
         self._action = nil
         self._script_info = nil
         self._attacker = nil
         self._start_time = nil
         self._end_time = nil
      end
   end
end
