local Fabricate = class()

Fabricate.name = 'fabricate'
Fabricate.does = 'stonehearth:fabricate'
Fabricate.priority = 5

function Fabricate:run(ai, entity, path)
   local endpoint = path:get_destination()
   local fabricator = endpoint:get_component('stonehearth:fabricator')
   local block = path:get_destination_point_of_interest()
   local carrying = radiant.entities.get_carrying(entity)
   
   ai:execute('stonehearth:follow_path', path)

   local entity_origin = radiant.entities.get_world_grid_location(entity)
   repeat
      radiant.entities.turn_to_face(entity, block)
      ai:execute('stonehearth:run_effect', 'work')
      fabricator:add_block(carrying, block)
      carrying = radiant.entities.consume_carrying(entity)
      block = fabricator:find_another_block(carrying, entity_origin)
   until not block
end

return Fabricate
