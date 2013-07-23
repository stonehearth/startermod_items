
function start_construction(player, data)
   local blueprint = radiant.entities.get_entity(data.blueprint_id)
   assert(building, string.format('no blueprint with entity id %d', data.blueprint_id))

   local building = radiant.entities.create_entity()
   local mapping = blueprint:clone_into(building)
end

return start_construction
