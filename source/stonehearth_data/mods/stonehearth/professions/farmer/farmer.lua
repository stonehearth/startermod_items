local farmer_class = {}

function farmer_class.promote(entity)
   local weapon = radiant.entities.create_entity('stonehearth:farmer:hoe')
   radiant.entities.equip_item(entity, weapon, 'melee_weapon')

   -- HACK: remove the talisman glow effect from the weapon
   radiant.entities.remove_effects(weapon)
end

function farmer_class.restore(entity)
end

function farmer_class.demote(entity)
end

return farmer_class
