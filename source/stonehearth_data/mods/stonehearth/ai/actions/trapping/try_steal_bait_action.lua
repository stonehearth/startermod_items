local Entity = _radiant.om.Entity

local TryStealBait = class()

TryStealBait.name = 'try steal bait'
TryStealBait.status_text = 'stealing bait'
TryStealBait.does = 'stonehearth:basic_needs'
TryStealBait.args = {}
TryStealBait.set_think_output = {
   trap = Entity
}
TryStealBait.version = 2
TryStealBait.priority = 1

function TryStealBait:start_thinking(ai, entity, args)
   local sensor_list = entity:add_component('sensor_list')
   local sensor = sensor_list:get_sensor('sight')
   assert(sensor)

   local trap = nil
   local trap_distance = nil

   for id, object in sensor:each_contents() do
      local trap_component = object:get_component('stonehearth:bait_trap')

      -- find the closest trap that can trap the entity
      if trap_component and trap_component:is_armed() then
         if trap_component:in_range(entity) and trap_component:can_trap(entity) then
            local object_distance = radiant.entities.distance_between(object, entity)
            if not trap or object_distance < trap_distance then
               trap = object
               trap_distance = object_distance                  
            end
         end
      end
   end

   if trap then
      ai:set_think_output({ trap = trap })
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(TryStealBait)
   :execute('stonehearth:goto_entity', {
      entity = ai.BACK(1).trap
   })
   :execute('stonehearth:turn_to_face_entity', {
      entity = ai.BACK(2).trap
   })
   :execute('stonehearth:run_effect', {
      effect = 'idle_look_around'
   })
   :execute('stonehearth:run_effect', {
      effect = 'eat',
      times = 1
   })
   :execute('stonehearth:trapping:trigger_trap', {
      trap = ai.BACK(5).trap
   })
   -- everything below will be pre-empted if the entity was trapped
   :execute('stonehearth:run_away_from_entity', {
      threat = ai.BACK(6).trap,
      distance = 32
   })
