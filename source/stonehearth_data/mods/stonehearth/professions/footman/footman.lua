local footman_class = {}

function footman_class.promote(entity, json, talisman_uri)
   local weapon = radiant.entities.create_entity(talisman_uri)
   radiant.entities.equip_item(entity, weapon, 'melee_weapon')

   -- HACK: remove the talisman glow effect from the weapon
   -- might want to remove other talisman related commands as well
   -- TODO: make the effects and commands specific to the model variant
   weapon:remove_component('effect_list')
end

function footman_class.demote(entity)
   -- TODO: unequip the weapon
   assert(false, 'not implemented')
end

return footman_class
