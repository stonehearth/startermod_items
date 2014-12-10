local LeaveAnimalInPasture = class()
local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3

LeaveAnimalInPasture.name = 'leave trailing animal in pasture'
LeaveAnimalInPasture.does = 'stonehearth:leave_animal_in_pasture'
LeaveAnimalInPasture.args = {
   animal = Entity,
   pasture = Point3
}
LeaveAnimalInPasture.version = 2
LeaveAnimalInPasture.priority = 1

--[[
--Only set think output when the critter is right beside us (follow logic from follow_entity)
--and we've gotten to our target destination
function LeaveAnimalInPasture:start_thinking(ai, entity, args)
   local log = ai:get_log()
   local target = args.animal
   local check_adjacent = function()
      local adjacent = radiant.entities.is_adjacent_to(entity, target)
      local at_target_location = false
      local entity_location = radiant.entities.get_world_grid_location(entity)
      if entity_location.x == args.pasture.x and entity_location.z == args.pasture.z then
         at_target_location = true
      end
      if adjacent and at_target_location then
         ai:set_think_output()
         if self._trace then
            self._trace:destroy()
            self._trace = nil
         end
         return true
      else
         return false
      end
   end

   local started = check_adjacent()
   if not started and not self._trace then
      self._trace = radiant.entities.trace_grid_location(target, 'wait for animal to be adjacent')
         :on_changed(function()
            check_adjacent()
         end)
         :on_destroyed(function()
            log:info('animal following was destroyed')
         end)
   end
end
]]

function LeaveAnimalInPasture:run(ai, entity, args)
   ai:execute('stonehearth:turn_to_face_entity', { entity = args.animal})
   ai:execute('stonehearth:run_effect', { effect = 'fiddle' })

   local pasture_tag = args.animal:get_component('stonehearth:equipment'):has_item_type('stonehearth:pasture_tag')
   local shepherded_component = pasture_tag:get_component('stonehearth:shepherded_animal')
   shepherded_component:set_following(false)

   local shepherd_class = entity:get_component('stonehearth:job'):get_curr_job_controller()
   shepherd_class:remove_trailing_animal(args.animal:get_id())
end

return LeaveAnimalInPasture