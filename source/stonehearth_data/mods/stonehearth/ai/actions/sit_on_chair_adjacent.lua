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
   local chair = args.chair

   self._current_location = entity:get_component('mob'):get_location()
   entity:get_component('mob'):move_to(chair:get_component('mob'):get_location())
   
   local q = chair:get_component('mob'):get_rotation()
   entity:get_component('mob'):set_rotation(q)

   radiant.entities.set_posture(entity, 'stonehearth:sitting_on_chair')
end

function SitOnChairAdjacent:stop(ai, entity, args)
   if self._current_location then
      entity:get_component('mob'):move_to(self._current_location)
      self._current_location = nil
   end
   radiant.entities.unset_posture(entity, 'stonehearth:sitting_on_chair')
end

return SitOnChairAdjacent
