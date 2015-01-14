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

function LeaveAnimalInPasture:run(ai, entity, args)
   
   local pasture_tag = args.animal:get_component('stonehearth:equipment'):has_item_type('stonehearth:pasture_tag')
   if pasture_tag then
      ai:execute('stonehearth:turn_to_face_entity', { entity = args.animal})
      ai:execute('stonehearth:run_effect', { effect = 'fiddle' })

      local shepherded_component = pasture_tag:get_component('stonehearth:shepherded_animal')
      shepherded_component:set_following(false)

      local shepherd_class = entity:get_component('stonehearth:job'):get_curr_job_controller()
      shepherd_class:remove_trailing_animal(args.animal:get_id())

      --Fire an event to let people know the animal has been returned to the pasture
      radiant.events.trigger_async(entity, 'stonehearth:leave_animal_in_pasture', {animal = args.animal})
   end
end

return LeaveAnimalInPasture