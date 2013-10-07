local Fabricate = class()

Fabricate.does = 'stonehearth.fabricate'
Fabricate.priority = 5

function Fabricate:run(ai, entity, path)
   local fabricator = path:get_destination():get_component('stonehearth:fabricator')
   local block = path:get_destination_point_of_interest()
   local carrying = radiant.entities.get_carrying(entity)
   
   ai:execute('stonehearth.follow_path', path)
   fabricator:add_block(carrying, block)
end

return Fabricate
