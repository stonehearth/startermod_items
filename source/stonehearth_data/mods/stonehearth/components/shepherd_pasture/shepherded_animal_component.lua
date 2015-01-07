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
   self._kill_listener:destroy()
   self._kill_listener = nil
end

--When we've been killed, remove ourselves from our pasture, if we have one
--Remove ourselves from the trailing shepherd, if that's appropriate
function ShepherdedAnimalComponent:_on_kill_event()
   self:free_animal()
end

function ShepherdedAnimalComponent:free_animal()
   --Free self from pasture
   if self._sv.pasture then
      local pasture_component = self._sv.pasture:get_component('stonehearth:shepherd_pasture')
      pasture_component:remove_animal(self._sv.animal:get_id())
      
   end

   --Free self from shepherd
   if self._sv.should_follow then
      if self._sv.last_shepherd_entity then
         local shepherd_class = self._sv.last_shepherd_entity:get_component('stonehearth:job'):get_curr_job_controller()
         if shepherd_class and shepherd_class.remove_trailing_animal then
            shepherd_class:remove_trailing_animal(self._sv.animal:get_id())
         end
         
      end
   end

   --Remove the pasture tag
   local equipment_component = self._sv.animal:get_component('stonehearth:equipment')
   local pasture_tag = equipment_component:has_item_type('stonehearth:pasture_tag')
   if pasture_tag then
      equipment_component:unequip_item('stonehearth:pasture_tag')
   end

   self._sv.pasture = nil
   self._sv.last_shepherd_entity = nil
end

--Remember the animal that this component is following
--The self._entity is the pasture tag. 
function ShepherdedAnimalComponent:set_animal(animal_entity)
   self._sv.animal = animal_entity
   self._sv.original_player_id = radiant.entities.get_player_id(animal_entity)
   self._kill_listener = radiant.events.listen(self._sv.animal, 'stonehearth:kill_event', self, self._on_kill_event)
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

   --If we're following a shepherd, make our animal's player_id equal to his player ID
   if should_follow then
      local shepherd_player_id = radiant.entities.get_player_id(shepherd)
      radiant.entities.set_player_id(self._sv.animal, shepherd_player_id)
   else
   --If we're not following, set it back
      radiant.entities.set_player_id(self._sv.animal, self._sv.original_player_id)
   end

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