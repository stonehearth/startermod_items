local Entity = _radiant.om.Entity

local SitOnChairAdjacent = class()
SitOnChairAdjacent.name = 'sit on chair adjacent'
SitOnChairAdjacent.does = 'stonehearth:sit_on_chair_adjacent'
SitOnChairAdjacent.args = {
   chair = Entity
}
SitOnChairAdjacent.version = 2
SitOnChairAdjacent.priority = 1

function SitOnChairAdjacent:run(ai, entity, args)
   local mob = entity:get_component('mob')
   local chair = args.chair

   local chair_location = radiant.entities.get_world_grid_location(chair)
   local chair_rotation = chair:get_component('mob')
                                 :get_rotation()

   self._current_location = radiant.entities.get_world_grid_location(entity)

   mob:set_rotation(chair_rotation)
      :move_to(chair_location)

   radiant.entities.set_posture(entity, 'stonehearth:sitting_on_chair')
end

function SitOnChairAdjacent:stop(ai, entity, args)
   if self._current_location then
      entity:get_component('mob')
               :move_to(self._current_location)
      self._current_location = nil
   end
   radiant.entities.unset_posture(entity, 'stonehearth:sitting_on_chair')
end

return SitOnChairAdjacent
