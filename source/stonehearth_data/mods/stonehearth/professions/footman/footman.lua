local footman_class = {}

function footman_class.promote(entity, json, talisman_uri)
   local weapon = radiant.entities.create_entity(talisman_uri)
   radiant.entities.equip_item(entity, weapon)

   stonehearth.combat:set_stance(entity, 'aggressive')
end

function footman_class.demote(entity)
   -- TODO: unequip the weapon
   assert(false, 'not implemented')
end

return footman_class
