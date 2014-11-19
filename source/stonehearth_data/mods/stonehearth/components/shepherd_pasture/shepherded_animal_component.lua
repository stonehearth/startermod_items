local ShepherdedAnimalComponent = class()

function ShepherdedAnimalComponent:initialize(entity, json)
   self._entity = entity
   if not self._sv.initialized then
      self._sv.initialized = true
      self._sv.pasture_id = nil
      self._sv.shouldFollowShepherd = true
   else
   end
end

function ShepherdedAnimalComponent:destroy()
end

function ShepherdedAnimalComponent:set_pasture(pasture_entity_id)
   self._sv.pasture_id = pasture_entity_id
end

function ShepherdedAnimalComponent:set_following(should_follow)
   self._sv.shouldFollowShepherd = should_follow
end

return ShepherdedAnimalComponent