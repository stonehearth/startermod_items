local footman_class = {}

function footman_class.promote(entity, json, talisman_uri)
   local weapon = radiant.entities.create_entity(talisman_uri)
   local equipment = entity:add_component('stonehearth:equipment')
   equipment:equip_item(weapon, 'mainHand')

   -- HACK: remove the talisman glow effect from the weapon
   -- might want to remove other talisman related commands as well
   -- TODO: make the effects and commands specific to the model variant
   weapon:remove_component('effect_list')
end

function footman_class.demote(entity)
   -- TODO: unequip the weapon
   assert('not implemented')
end

return footman_class
