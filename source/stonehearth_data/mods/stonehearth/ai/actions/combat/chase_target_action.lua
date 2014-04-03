local Analytics = stonehearth.analytics

local ChaseTarget = class()

ChaseTarget.name = 'chase target'
ChaseTarget.does = 'stonehearth:attack:chase_target'
ChaseTarget.version = 1
ChaseTarget.priority = 0

function ChaseTarget:run(ai, entity, target, effect_name)
   --Get some logging in here
   --TODO: replace entity_name with entity_description
   local entity_name = entity:get_component('unit_info'):get_display_name()
   local target_name = entity:get_component('unit_info'):get_display_name()

   if not target then
      target = self._target
   end

   assert(target)
   ai:execute('stonehearth:run_toward_entity', target, effect_name)

   --TODO: move to a death event, if such occurs.
   --Analytics:send_design_event('game:craft', entity, target)
end

return ChaseTarget