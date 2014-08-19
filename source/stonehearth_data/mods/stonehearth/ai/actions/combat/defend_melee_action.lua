local rng = _radiant.csg.get_default_rng()
local log = radiant.log.create_logger('combat')

local DefendMelee = class()

DefendMelee.name = 'defend melee'
DefendMelee.does = 'stonehearth:combat:defend'
DefendMelee.args = {}
DefendMelee.version = 2
DefendMelee.priority = 2
DefendMelee.weight = 1

function DefendMelee:__init(entity)
end

function DefendMelee:start_thinking(ai, entity, args)
   self._defense_scheduled = false
   self._ai = ai
   self._entity = entity

   -- refetch every start_thinking as the set of actions may have changed
   self._defense_types = stonehearth.combat:get_combat_actions(entity, 'stonehearth:combat:melee_defenses')

   -- TODO: refactor all this assault checking and defense scheduling code so that we can reuse it for ranged/spell/etc defense
   local assault_events = stonehearth.combat:get_assault_events(entity)

   for _, assault_context in pairs(assault_events) do
      self:_on_assault(assault_context)
      if self._defense_scheduled then
         return
      end
   end

   self:_add_assault_trace()
end

function DefendMelee:stop_thinking(ai, entity, args)
   self._ai = nil
   self._entity = nil

   self:_remove_assault_trace()
   self:_destroy_timer()
end

function DefendMelee:_add_assault_trace()
    self._assault_trace = radiant.events.listen(self._entity, 'stonehearth:combat:assault', self, self._on_assault)
end

function DefendMelee:_remove_assault_trace()
   if self._assault_trace then
      self._assault_trace:destroy()
      self._assault_trace = nil
   end
end

function DefendMelee:_destroy_timer()
   if self._timer then
      self._timer:destroy()
      self._timer = nil
   end
end

function DefendMelee:_on_assault(assault_context)
   if assault_context.attack_method ~= 'melee' then
      return
   end

   local defend_info = stonehearth.combat:choose_defense_action(self._entity, self._defense_types, assault_context.impact_time)
   if defend_info then
      self:_remove_assault_trace()
      self:_schedule_defense(assault_context, defend_info)
      self._defense_scheduled = true
   end
end

function DefendMelee:_schedule_defense(assault_context, defend_info)
   local buffer_time = 100
   local defend_start_time = assault_context.impact_time - defend_info.time_to_impact
   local defend_start_delay = defend_start_time - radiant.gamestate.now()

   self._assault_context = assault_context
   self._defend_info = defend_info

   if defend_start_delay <= buffer_time then
      self._ai:set_think_output()
   else
      -- allow other actions to run until we need to start defending
      -- buffer time ensures that we have enough time to get to run before starting the effect
      self._timer = stonehearth.combat:set_timer(defend_start_delay - buffer_time,
         function()
            if self._assault_context.assault_active then
               self._ai:set_think_output()
            end
         end
      )
   end
end

function DefendMelee:run(ai, entity, args)
   if not self._assault_context.assault_active then
      ai:abort('assault was cancelled')
   end

   -- make sure we still have time to defend
   local defend_start_time = self._assault_context.impact_time - self._defend_info.time_to_impact
   local defend_start_delay = defend_start_time - radiant.gamestate.now()
   
   if defend_start_delay < 0 then
      log:warning('DefendMelee could not run in time')
      return
   end

   self._assault_context.target_defending = true
   
   radiant.entities.turn_to_face(entity, self._assault_context.attacker)

   stonehearth.combat:start_cooldown(entity, self._defend_info)

   ai:execute('stonehearth:run_effect', {
      effect = self._defend_info.name,
      delay = defend_start_delay
   })
end

function DefendMelee:stop(ai, entity, args)
   -- signal attacker if the defense was interrupted before the impact time
   if radiant.gamestate.now() < self._assault_context.impact_time then
      self._assault_context.target_defending = false
   end

   self._assault_context = nil
end

return DefendMelee
