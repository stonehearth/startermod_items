local farmer_class = {}

function farmer_class.promote(entity)
   local weapon = radiant.entities.create_entity('stonehearth:farmer:hoe')
   radiant.entities.equip_item(entity, weapon)
   stonehearth.combat:set_stance(entity, 'passive')
end

function farmer_class.restore(entity)
end

function farmer_class.demote(entity)
end

return farmer_class
