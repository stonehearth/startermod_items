local Entity = _radiant.om.Entity
local rng = _radiant.csg.get_default_rng()
local log = radiant.log.create_logger('combat')

local DefendMelee = class()

DefendMelee.name = 'melee defense'
DefendMelee.does = 'stonehearth:combat:defend'
DefendMelee.args = {}
DefendMelee.version = 2
DefendMelee.priority = 1
DefendMelee.weight = 1

function DefendMelee:__init(entity)
   self._defense_types = stonehearth.combat:get_action_types(entity, 'stonehearth:combat:melee_defenses')
end

function DefendMelee:start_thinking(ai, entity, args)
   self._ai = ai
   self._entity = entity
   self._think_output_set = false

   local assault_events = stonehearth.combat:get_assault_events(entity)

   for _, context in ipairs(assault_events) do
      self:_on_assault(context)
      if self._think_output_set then
         return
      end
   end

   self:_register_events()
end

function DefendMelee:stop_thinking(ai, entity, args)
   self:_unregister_events()

   self._ai = nil
   self._entity = nil
end

function DefendMelee:_register_events()
   if not self._registered then
      radiant.events.listen(self._entity, 'stonehearth:combat:assault', self, self._on_assault)
      self._registered = true
   end
end

function DefendMelee:_unregister_events()
   if self._registered then
      radiant.events.unlisten(self._entity, 'stonehearth:combat:assault', self, self._on_assault)
      self._registered = false
   end
end

function DefendMelee:_on_assault(context)
   if self._think_output_set then
      return
   end

   if context.attack_method ~= 'melee' then
      return
   end

   local defend_info = stonehearth.combat:choose_action(self._entity, self._defense_types)
   local defend_latency = stonehearth.combat:get_time_to_impact(defend_info)
   local earliest_defend_time = radiant.gamestate.now() + defend_latency

   if earliest_defend_time > context.impact_time then
      -- too slow, cannot defend
      return
   end

   self._context = context
   self._defend_latency = defend_latency
   self._defend_info = defend_info

   self:_set_think_output()
end

function DefendMelee:_set_think_output()
   if not self._think_output_set then
      self._ai:set_think_output() -- CHECKCHECK
      self._think_output_set = true
   end
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
