local trapper_class = {}

function trapper_class.promote(entity, json, talisman_uri)
   stonehearth.combat:set_stance(entity, 'defensive')
end

function trapper_class.demote(entity)
end

return trapper_class
