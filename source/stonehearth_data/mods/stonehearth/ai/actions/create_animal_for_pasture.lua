local Entity = _radiant.om.Entity

local CreateAnimalForPasture = class()

CreateAnimalForPasture.name = 'create animal for pasture'
CreateAnimalForPasture.does = 'stonehearth:create_animal_for_pasture'
CreateAnimalForPasture.args = {
   pasture = Entity
}
CreateAnimalForPasture.think_output = {
   animal = Entity
}
CreateAnimalForPasture.version = 2
CreateAnimalForPasture.priority = 1

-- Generate an animal of the appropriate type somewhere near the edge of the
-- explored region. If the shepherd cannot yet "make" an animal to put in 
-- the world, then create a decoy animal
-- TODO: link the spawn location to the terrain type
-- TODO: gameplay: what if the player can specify where the shepherd should search?

function CreateAnimalForPasture:start_thinking(ai, entity, args)
   self._thinking_started = true
   self._args = args
   self._ai = ai
   self._entity = entity
   
   self:_try_shepherd_spawn()
end

--Recursively call self till we successfully get an animal to spawn
function CreateAnimalForPasture:_try_shepherd_spawn()
   local shepherd_class = self._entity:get_component('stonehearth:job'):get_curr_job_controller()
   if shepherd_class and shepherd_class.can_find_animal_in_world then
      local can_find_real_critter = shepherd_class:can_find_animal_in_world()
      self:_try_spawning_animal(can_find_real_critter)
      if not can_find_real_critter then
         self._timer = stonehearth.calendar:set_timer('1h', function()
            if self._thinking_started then
               self:_try_shepherd_spawn()
            end
         end)
      end
   end
end

-- Try to put the animal somewhere on the perimeter
-- @param real_animal - true if it's a real animal that the shepherd found, false if it's a false alarm
function CreateAnimalForPasture:_try_spawning_animal(real_animal)
   local pasture_component = self._args.pasture:get_component('stonehearth:shepherd_pasture')
   local animal_type = pasture_component:get_pasture_type()
   local animal
   if real_animal then
      animal = radiant.entities.create_entity(animal_type)
   else
      animal = radiant.entities.create_entity()
      self._decoy = animal
   end
   local player_id = radiant.entities.get_player_id(self._entity)
   local camp_standard = stonehearth.town:get_town(player_id):get_banner()
   
   --TODO: is this async? How long does this take?
   stonehearth.spawn_region_finder:find_point_outside_civ_perimeter_for_entity_astar(
      animal, self._args.pasture, 20, 5, 500,
      function (spawn_point)
         --Success function
         if self._thinking_started then
            radiant.terrain.place_entity(animal, spawn_point)
            self._ai:set_think_output({animal = animal})
         else
            radiant.entities.destroy_entity(animal)
         end
      end, 
      function ()
         --Fail function, but the action is still going, so try again
         radiant.entities.destroy_entity(animal)
         self:_try_spawning_later('1h')
      end)
end

--If we couldn't find a place to put the animal, try again later
function CreateAnimalForPasture:_try_spawning_later(time)
   self._timer = stonehearth.calendar:set_timer(time, function()
      if self._thinking_started then
         self:_try_spawning_animal()
      end
      end)
end

--If we stop thinking before our timers or finders finish, stop them
function CreateAnimalForPasture:stop_thinking(ai, entity, args)
   self._thinking_started = false
   if self._timer then
      self._timer:destroy()
      self._timer = nil
   end
end

--Should be called after the whole stack unwinds, after we actually get to the entity
--if it's a decoy entity, destroy it
function CreateAnimalForPasture:stop()
   if self._decoy then 
      radiant.entities.destroy_entity(self._decoy)
      self._decoy = nil
   end
end

return CreateAnimalForPasture