local ShepherdedAnimalComponent = class()

function ShepherdedAnimalComponent:initialize(entity, json)
   self._entity = entity  --this is the pasture tag, not the actual animal
   if not self._sv.initialized then
      self._sv.initialized = true
      self._sv.pasture = nil
      self._sv.should_follow = false
      self._sv.last_shepherd_entity = false
   else
   end
end

function ShepherdedAnimalComponent:destroy()
end

--Remember the animal that this component is following
--The self._entity is the pasture tag. 
function ShepherdedAnimalComponent:set_animal(animal_entity)
   self._sv.animal = animal_entity
end

function ShepherdedAnimalComponent:set_pasture(pasture_entity)
   self._sv.pasture = pasture_entity
end

-- Whether the critter should follow a shepherd around
-- @param should_follow - true if yes, false if no
-- @param shepherd - the shepherd entity that the critter is following,nil if false
function ShepherdedAnimalComponent:set_following(should_follow, shepherd)
   self._sv.should_follow = should_follow
   self._sv.last_shepherd_entity = shepherd

   radiant.events.trigger(self._sv.animal, 'stonehearth:shepherded_animal_follow_status_change', 
      {should_follow = self._sv.should_follow, 
       shepherd = shepherd})
end

function ShepherdedAnimalComponent:get_following()
   return self._sv.should_follow
end

function ShepherdedAnimalComponent:get_last_shepherd()
   return self._sv.last_shepherd_entity
end

function ShepherdedAnimalComponent:get_pasture()
   return self._sv.pasture
end

return ShepherdedAnimalComponent