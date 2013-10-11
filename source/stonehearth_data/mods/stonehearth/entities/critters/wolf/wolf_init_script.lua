local teeth = radiant.entities.create_entity('stonehearth:wolf:teeth_weapon')

local init_fn = function (entity)
   entity:add_component('stonehearth:equipment'):equip_item(teeth)
end

return init_fn

