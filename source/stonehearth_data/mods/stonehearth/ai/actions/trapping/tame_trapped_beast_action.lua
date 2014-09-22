local Entity = _radiant.om.Entity

local TameTrappedBeast = class()

TameTrappedBeast.name = 'tame trapped beast'
TameTrappedBeast.does = 'stonehearth:trapping:check_bait_trap_adjacent'
TameTrappedBeast.args = {
   trap = Entity
}
TameTrappedBeast.version = 2
TameTrappedBeast.priority = 1
TameTrappedBeast.weight = 1

function TameTrappedBeast:start_thinking(ai, entity, args)
   -- not allowing entities to own more than one pet at a time
   if entity:add_component('stonehearth:pet_owner'):num_pets() > 0 then
      return
   end

   local trap_component = args.trap:add_component('stonehearth:bait_trap')
   if trap_component:get_trapped_entity() then
      ai:set_think_output()
   end
end

function TameTrappedBeast:run(ai, entity, args)
   local trap_component = args.trap:add_component('stonehearth:bait_trap')
   local pet = trap_component:get_trapped_entity()

   ai:execute('stonehearth:turn_to_face_entity', { entity = args.trap })
   ai:execute('stonehearth:run_effect', { effect = 'fiddle' })

   if pet and pet:is_valid() then
      local equipment_component = pet:add_component('stonehearth:equipment')
      local pet_collar = radiant.entities.create_entity('stonehearth:pet_collar')
      equipment_component:equip_item(pet_collar)

      entity:add_component('stonehearth:pet_owner'):add_pet(pet)

      local town = stonehearth.town:get_town(entity)
      town:add_pet(pet)

      -- adjust the pet's name and description to indicate that it's a pet. xxx, localize
      radiant.entities.set_name(pet, 'Pet ' .. radiant.entities.get_name(pet))
      radiant.entities.set_description(pet, 'Befriended by ' .. radiant.entities.get_name(entity))

      trap_component:release()
   end

   ai:unprotect_entity(args.trap)
   radiant.entities.destroy_entity(args.trap)
end

return TameTrappedBeast
