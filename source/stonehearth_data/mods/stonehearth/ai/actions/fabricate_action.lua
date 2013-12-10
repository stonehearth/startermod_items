local Fabricate = class()

Fabricate.does = 'stonehearth:fabricate'
Fabricate.priority = 5

function Fabricate:run(ai, entity, path)
   local endpoint = path:get_destination()
   local fabricator = endpoint:get_component('stonehearth:fabricator')
   local block = path:get_destination_point_of_interest()
   local carrying = radiant.entities.get_carrying(entity)
   
   ai:execute('stonehearth:follow_path', path)
   radiant.entities.turn_to_face(entity, block)
   ai:execute('stonehearth:run_effect', 'work')

   fabricator:add_block(carrying, block)
   radiant.entities.consume_carrying(entity)
end

return Fabricate
