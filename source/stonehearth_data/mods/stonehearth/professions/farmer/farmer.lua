local farmer_class = {}

function farmer_class.promote(entity)
   stonehearth.combat:set_stance(entity, 'passive')
end

function farmer_class.restore(entity)
end

function farmer_class.demote(entity)
end

return farmer_class
