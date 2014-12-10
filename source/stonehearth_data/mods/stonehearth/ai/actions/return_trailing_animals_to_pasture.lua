local ReturnTrailingAnimalToPasture = class()

ReturnTrailingAnimalToPasture.name = 'return trailing animal to pasture'
ReturnTrailingAnimalToPasture.does = 'stonehearth:herding'
ReturnTrailingAnimalToPasture.args = {}
ReturnTrailingAnimalToPasture.version = 2
ReturnTrailingAnimalToPasture.priority = stonehearth.constants.priorities.shepherding_animals.RETURN_TO_PASTURE

--We should only run if we are trailing animals
function ReturnTrailingAnimalToPasture:start_thinking(ai, entity, args)
   local shepherd_class = entity:get_component('stonehearth:job'):get_curr_job_controller()
   if shepherd_class and shepherd_class.get_trailing_animals then
      local trailed_animals, num_trailed = shepherd_class:get_trailing_animals()
      if num_trailed and num_trailed > 0 then
         ai:set_think_output()
      end
   end
   --TODO: confirm this is working after it exits once?
end

--Go to the pasture associated with the first entity
--Drop off the sheep there.
--TODO: fix so goto location is instead inside a compound action
function ReturnTrailingAnimalToPasture:run(ai, entity, args)
   local shepherd_class = entity:get_component('stonehearth:job'):get_curr_job_controller()
   if shepherd_class and shepherd_class.add_trailing_animal then
      local trailed_animals, num_trailed = shepherd_class:get_trailing_animals()
      if num_trailed and num_trailed > 0 then
         for id, animal in pairs(trailed_animals) do
            --Find the pasture
            local pasture_tag = animal:get_component('stonehearth:equipment'):has_item_type('stonehearth:pasture_tag')
            local shepherded_component = pasture_tag:get_component('stonehearth:shepherded_animal')
            local pasture = shepherded_component:get_pasture()

            local pasture_component = pasture:get_component('stonehearth:shepherd_pasture')
            ai:execute('stonehearth:goto_location', {
               reason = 'dropping off some critters!',
               location = pasture_component:get_center_point()
               })

            --If the animal is still with us
            if shepherded_component then
               -- Can we wait here till the animal catches up with us? 
               ai:execute('stonehearth:run_effect', {effect = 'idle_look_around'})
               --ai:execute('stonehearth:goto_entity', {entity = animal})
               ai:execute('stonehearth:run_effect', { effect = 'fiddle' })
               shepherded_component:set_following(false)
               shepherd_class:remove_trailing_animal(animal:get_id())
            end

            --We could keep going, but let's break to let the AI run again
            break
         end

      end
   end
end

return ReturnTrailingAnimalToPasture