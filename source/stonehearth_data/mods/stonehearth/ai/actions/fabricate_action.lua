local Fabricate = class()

Fabricate.name = 'fabricate'
Fabricate.does = 'stonehearth:fabricate'
Fabricate.priority = 5

function Fabricate:run(ai, entity, path)
   local endpoint = path:get_destination()
   local carrying = radiant.entities.get_carrying(entity)

   ai:execute('stonehearth:follow_path', path)

   self._fabricator = endpoint:get_component('stonehearth:fabricator')
   self._current_block = path:get_destination_point_of_interest()  
   
   local entity_origin = radiant.entities.get_world_grid_location(entity)
   repeat
      radiant.entities.turn_to_face(entity, self._current_block)
      ai:execute('stonehearth:run_effect', 'work')
      self._fabricator:add_block(carrying, self._current_block)

      carrying = radiant.entities.consume_carrying(entity)
      self._current_block = self._fabricator:find_another_block(carrying, entity_origin)
   until not self._current_block
end

function Fabricate:stop(ai, entity)
   if self._current_block then
      self._fabricator:release_block(self._current_block)
      self._current_block = nil
   end
end


return Fabricate
