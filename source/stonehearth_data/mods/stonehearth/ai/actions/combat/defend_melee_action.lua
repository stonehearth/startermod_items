local rng = _radiant.csg.get_default_rng()
local log = radiant.log.create_logger('combat')

local DefendMelee = class()

DefendMelee.name = 'defend melee'
DefendMelee.does = 'stonehearth:combat:defend'
DefendMelee.args = {
   assault_events = 'table'   -- an array of assault events (AssaultContexts)
}
DefendMelee.version = 2
DefendMelee.priority = 2
DefendMelee.weight = 1

function DefendMelee:__init(entity)
   self._defense_types = stonehearth.combat:get_combat_actions(entity, 'stonehearth:combat:melee_defenses')
end

function DefendMelee:start_thinking(ai, entity, args)
   self._ai = ai
   self._entity = entity

   if next(self._defense_types) == nil then
      -- no melee defenses
      return
   end

   for _, context in pairs(args.assault_events) do
      if self:_can_defend(context) then
         ai:set_think_output()
      end
   end
end

function DefendMelee:stop_thinking(ai, entity, args)
   self._ai = nil
   self._entity = nil
end

function DefendMelee:_can_defend(context)
   if context.attack_method ~= 'melee' then
      return false
   end

   local defend_info = stonehearth.combat:choose_combat_action(self._entity, self._defense_types)

   if not defend_info then
      -- no defenses currently available
      return false
   end

   local defend_latency = stonehearth.combat:get_time_to_impact(defend_info)
   local earliest_defend_time = radiant.gamestate.now() + defend_latency

   if earliest_defend_time > context.impact_time then
      -- too slow, cannot defend
      return false
   end

   self._context = context
   self._defend_latency = defend_latency
   self._defend_info = defend_info

   return true
end

function DefendMelee:run(ai, entity, args)
   -- make sure we still have time to defend
   local time_to_impact = self._context.impact_time - radiant.gamestate.now()
   local defend_delay = time_to_impact - self._defend_latency
   
   if defend_delay < 0 then
      -- nope!
      return
   end

   self._context.target_defending = true
   
   radiant.entities.turn_to_face(entity, self._context.attacker)

   stonehearth.combat:start_cooldown(entity, self._defend_info)

   ai:execute('stonehearth:run_effect', {
      effect = self._defend_info.name,
      delay = defend_delay
   })
end

function DefendMelee:stop(ai, entity, args)
   -- signal attacker if the defense was interrupted before the impact time
   if radiant.gamestate.now() < self._context.impact_time then
      self._context.target_defending = false
   end

   self._context = nil
end

return DefendMelee
