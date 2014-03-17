local TameTrappedBeastAdjacent = class()

local Entity = _radiant.om.Entity

TameTrappedBeastAdjacent.name = 'tame trapped beast adjacent'
TameTrappedBeastAdjacent.does = 'stonehearth:unit_control:tame_trapped_beast_adjacent'
TameTrappedBeastAdjacent.args = {
   trap = Entity
}
TameTrappedBeastAdjacent.version = 2
TameTrappedBeastAdjacent.priority = stonehearth.constants.priorities.unit_control.CAST_SPELL

function TameTrappedBeastAdjacent:start(ai, entity, args)
   self._entity = entity
end

function TameTrappedBeastAdjacent:run(ai, entity, args)
   local trap = args.trap
   
   if not trap or not trap:is_valid() then
      return
   end
   
   local _, pet = trap:get_component('entity_container'):first_child()

   if pet and pet:is_valid() then
      radiant.entities.remove_buff(pet, 'stonehearth:buffs:snared')

      local equipment = pet:add_component('stonehearth:equipment')
      equipment:equip_item('stonehearth:pet_collar')

      local faction = radiant.entities.get_faction(self._entity)
      pet:add_component('unit_info'):set_faction(faction)

      local town = stonehearth.town:get_town(self._entity)
      town:add_pet(pet)
   end
end

return TameTrappedBeastAdjacent
