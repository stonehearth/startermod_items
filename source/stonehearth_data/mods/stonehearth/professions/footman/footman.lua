local footman_class = {}

function footman_class.promote(entity, json, talisman_uri)
   stonehearth.combat:set_stance(entity, 'aggressive')
end

function footman_class.demote(entity)
   -- TODO: unequip the weapon
   assert(false, 'not implemented')
end

return footman_class
