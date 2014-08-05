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
   if not args.trap:is_valid() then
      return
   end

   local trap_component = args.trap:add_component('stonehearth:bait_trap')
   if trap_component:get_trapped_entity() then
      ai:set_think_output()
   end
end

function TameTrappedBeast:run(ai, entity, args)
   if not args.trap:is_valid() then
      self:stop()
      ai:abort('trap is destroyed')
      return
   end

   local trap_component = args.trap:add_component('stonehearth:bait_trap')
   local pet = trap_component:get_trapped_entity()

   ai:execute('stonehearth:turn_to_face_entity', { entity = args.trap })
   ai:execute('stonehearth:run_effect', { effect = 'fiddle' })

   if pet and pet:is_valid() then
      local equipment_component = pet:add_component('stonehearth:equipment')
      local pet_collar = radiant.entities.create_entity('stonehearth:pet_collar')
      equipment_component:equip_item(pet_collar)

      local faction = radiant.entities.get_faction(entity)
      pet:add_component('unit_info'):set_faction(faction)

      local town = stonehearth.town:get_town(entity)
      town:add_pet(pet)

      trap_component:release()
   end

   radiant.entities.destroy_entity(args.trap)
end

return TameTrappedBeast
