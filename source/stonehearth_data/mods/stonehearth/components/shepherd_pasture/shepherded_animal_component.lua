local ShepherdedAnimalComponent = class()

function ShepherdedAnimalComponent:initialize(entity, json)
   self._entity = entity
   if not self._sv.initialized then
      self._sv.initialized = true
      self._sv.pasture_id = nil
      self._sv.shouldFollowShepherd = false
      self._sv.last_shepherd_entity = false
   else
   end
end

function ShepherdedAnimalComponent:destroy()
end

function ShepherdedAnimalComponent:set_pasture(pasture_entity_id)
   self._sv.pasture_id = pasture_entity_id
end

-- Whether the critter should follow a shepherd around
-- @param should_follow - true if yes, false if no
-- @param shepherd - the shepherd entity that the critter is following,nil if false
function ShepherdedAnimalComponent:set_following(should_follow, shepherd)
   self._sv.should_follow = should_follow
   self._sv.last_shepherd_entity = shepherd
end

function ShepherdedAnimalComponent:get_following()
   return self._sv.should_follow
end

function ShepherdedAnimalComponent:get_last_shepherd()
   return self._sv.last_shepherd_entity
end

return ShepherdedAnimalComponent