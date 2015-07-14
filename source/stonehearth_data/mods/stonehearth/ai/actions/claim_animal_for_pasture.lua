local Entity = _radiant.om.Entity

local ClaimAnimalForPasture = class()
ClaimAnimalForPasture.name = 'claim animal for pasture'
ClaimAnimalForPasture.status_text_key = 'ai_status_text_claiming_animal'
ClaimAnimalForPasture.does = 'stonehearth:claim_animal_for_pasture'
ClaimAnimalForPasture.args = {
   pasture = Entity, 
   animal = Entity
}
ClaimAnimalForPasture.version = 2
ClaimAnimalForPasture.priority = 1

--If the animal isn't part of a pasture, claim it for a pasture
--Then make sure the animal will follow us home

function ClaimAnimalForPasture:run(ai, entity, args)
   local equipment_component = args.animal:add_component('stonehearth:equipment')

   local pasture_collar = equipment_component:has_item_type('stonehearth:pasture_tag')
   local first_catch = false
   if not pasture_collar then
      pasture_collar = radiant.entities.create_entity('stonehearth:pasture_tag')
      equipment_component:equip_item(pasture_collar)
      first_catch = true
   end

   local shepherded_animal_component = pasture_collar:get_component('stonehearth:shepherded_animal')
   if first_catch then
      shepherded_animal_component:set_animal(args.animal)
      shepherded_animal_component:set_pasture(args.pasture)

      self:_set_to_animal_follow(shepherded_animal_component, entity, args)
      
      local pasture_component = args.pasture:get_component('stonehearth:shepherd_pasture')
      pasture_component:add_animal(args.animal)
      
      radiant.events.trigger_async(entity, 'stonehearth:tame_animal', {animal = args.animal})
   else
      self:_set_to_animal_follow(shepherded_animal_component, entity, args)
   end

   ai:unprotect_argument(args.animal)
end

function ClaimAnimalForPasture:_set_to_animal_follow(shepherded_animal_component, entity, args)
   shepherded_animal_component:set_following(true, entity)
   --The shepherd is now trailing a sheep and will do so till he drops it off at the pasture
   --or until something kills it/steals it from him
   local shepherd_class = entity:get_component('stonehearth:job'):get_curr_job_controller()
   if shepherd_class and shepherd_class.add_trailing_animal then
      shepherd_class:add_trailing_animal(args.animal, args.pasture)
   end
end

return ClaimAnimalForPasture