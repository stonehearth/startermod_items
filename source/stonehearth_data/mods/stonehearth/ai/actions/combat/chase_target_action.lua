local ChaseTarget = class()

ChaseTarget.name = 'chase target'
ChaseTarget.does = 'stonehearth:attack:chase_target'
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
   ai:execute('stonehearth:run_towards_entity', target, effect_name)

   _radiant.analytics.DesignEvent('game:chase:' .. entity_name .. ':' .. target_name)

end

return ChaseTarget