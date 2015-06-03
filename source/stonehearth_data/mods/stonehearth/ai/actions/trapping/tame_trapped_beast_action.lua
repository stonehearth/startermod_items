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
   -- hearthlings with more compassion can own more pets
   local num_pets = entity:add_component('stonehearth:pet_owner'):num_pets()
   local max_num_pets = 1
   local attributes = entity:get_component('stonehearth:attributes')
   if attributes then
      local compassion = attributes:get_attribute('compassion')
      if compassion >= stonehearth.constants.attribute_effects.COMPASSION_TRAPPER_TWO_PETS_THRESHOLD then
         max_num_pets = 2
      end
   end

   if num_pets >= max_num_pets then
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
   local pet_id = nil

   ai:execute('stonehearth:turn_to_face_entity', { entity = args.trap })
   ai:execute('stonehearth:run_effect', { effect = 'fiddle' })

   if pet and pet:is_valid() then
      pet_id = pet:get_id()
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

      radiant.events.trigger_async(entity, 'stonehearth:befriend_pet', {pet_id = pet_id})
   end

   ai:unprotect_argument(args.trap)
   radiant.entities.destroy_entity(args.trap)
end

return TameTrappedBeast
