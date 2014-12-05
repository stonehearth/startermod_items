local Entity = _radiant.om.Entity

local ClaimAnimalForPasture = class()
ClaimAnimalForPasture.name = 'claim animal for pasture'
ClaimAnimalForPasture.does = 'stonehearth:claim_animal_for_pasture'
ClaimAnimalForPasture.args = {
   pasture = Entity, 
   animal = Entity
}
ClaimAnimalForPasture.version = 2
ClaimAnimalForPasture.priority = 1

function ClaimAnimalForPasture:run(ai, entity, args)
   local equipment_component = args.animal:add_component('stonehearth:equipment')

   local pasture_collar = radiant.entities.create_entity('stonehearth:pasture_tag')
   equipment_component:equip_item(pasture_collar)
   
   local shepherded_animal_component = pasture_collar:get_component('stonehearth:shepherded_animal')
   shepherded_animal_component:set_animal(args.animal)
   shepherded_animal_component:set_following(true, entity)
   shepherded_animal_component:set_pasture(args.pasture)

   local pasture_component = args.pasture:get_component('stonehearth:shepherd_pasture')
   pasture_component:add_animal(args.animal)

   --The shepherd is now trailing a sheep and will do so till he drops it off at the pasture
   --or until something kills it/steals it from him
   local shepherd_class = entity:get_component('stonehearth:job'):get_curr_job_controller()
   if shepherd_class and shepherd_class.add_trailing_animal then
      shepherd_class:add_trailing_animal(args.animal, args.pasture)
   end

   --TODO: action for sheep to follow dude till dude releases him. 
   ai:unprotect_entity(args.animal)
end

return ClaimAnimalForPasture