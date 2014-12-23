local ShepherdClass = class()
local job_helper = require 'jobs.job_helper'
local rng = _radiant.csg.get_default_rng()

--- Public functions, required for all classes

function ShepherdClass:initialize(entity)
   job_helper.initialize(self._sv, entity)
   self._sv.last_found_critter_time = nil

   self:restore()
end

function ShepherdClass:restore()
   self._job_component = self._sv._entity:get_component('stonehearth:job')

   --If we load and we're the current class, do these things
   if self._sv.is_current_class then
      --self:_create_xp_listeners()
   end

   self.__saved_variables:mark_changed()
end

function ShepherdClass:promote(json)
   job_helper.promote(self._sv, json)
   --self:_create_xp_listeners()

   self.__saved_variables:mark_changed()
end

-- Returns the level the character has in this class
function ShepherdClass:get_job_level()
   return self._sv.last_gained_lv
end

-- Returns whether we're at max level.
-- NOTE: If max level is nto declared, always false
function ShepherdClass:is_max_level()
   return self._sv.is_max_level 
end

-- Returns all the data for all the levels
function ShepherdClass:get_level_data()
   return self._sv.level_data
end

-- We keep an index of perks we've unlocked for easy lookup
function ShepherdClass:unlock_perk(id)
   self._sv.attained_perks[id] = true
   self.__saved_variables:mark_changed()
end

-- Given the ID of a perk, find out if we have the perk. 
function ShepherdClass:has_perk(id)
   return self._sv.attained_perks[id]
end

-- Call when it's time to level up in this class
function ShepherdClass:level_up()
   job_helper.level_up(self._sv)
   self.__saved_variables:mark_changed()
end

function ShepherdClass:demote()
   --self:_remove_xp_listeners()
   self._sv.is_current_class = false

   --Orphan all the animals
   self:_abandon_following_animals()

   --remove posture

   self.__saved_variables:mark_changed()

end

-- Shepherd related functionality

-- Add an animal to the list following the shepherd
function ShepherdClass:add_trailing_animal(animal, pasture)
   if not self._sv.trailed_animals then
      self._sv.trailed_animals = {}
      self._sv.num_trailed_animals = 0
   end
   self._sv.trailed_animals[animal:get_id()] = animal
   self._sv.num_trailed_animals = self._sv.num_trailed_animals + 1

   --If we have more than 1 animal, make sure we apply the shepherding speed debuf
   if self._sv.num_trailed_animals == 1 then   
      radiant.entities.set_posture(self._sv._entity, 'stonehearth:patrol')
      radiant.entities.add_buff(self._sv._entity, 'stonehearth:buffs:shepherding');
   end 

   --Fire an event saying that we've collected an animal
   radiant.events.trigger(self._sv._entity, 'stonehearth:add_trailing_animal', {animal = animal, pasture = pasture})
end

function ShepherdClass:get_trailing_animals()
   return self._sv.trailed_animals, self._sv.num_trailed_animals
end

function ShepherdClass:remove_trailing_animal(animal_id)
   if not self._sv.trailed_animals then
      return
   end
   self._sv.trailed_animals[animal_id] = nil
   self._sv.num_trailed_animals = self._sv.num_trailed_animals - 1

   assert(self._sv.num_trailed_animals >= 0, 'shepherd trying to remove animals he does not have')

--If we have no animals, remove the shepherding speed debuf
   if self._sv.num_trailed_animals == 0 then
      radiant.entities.remove_buff(self._sv._entity, 'stonehearth:buffs:shepherding')
      radiant.entities.unset_posture(self._sv._entity, 'stonehearth:patrol')
   end 
end

--Returns true if the shepherd was able to find an animal, false otherwise
--Depends on % bonus chance that increases as Shepherd levels, and time since last
--critter was found. 
--Starting shepherd: 100% chance if we've never found a critter before
--0% chance if we JUST found another critter
--100% chance if 24 in game hours have past since the last critter
--As shepherd levels up, bonus is added to % chance
function ShepherdClass:can_find_animal_in_world()
   local curr_elapsed_time = stonehearth.calendar:get_elapsed_time()
   local constants = stonehearth.calendar:get_constants()
   if not self._sv.last_found_critter_time then
      self._sv.last_found_critter_time = curr_elapsed_time
      return true
   else
      --Calc difference between curr time and last found critter time
      local elapsed_difference = curr_elapsed_time - self._sv.last_found_critter_time
      --convert ms to hours
      local elapsed_hours = elapsed_difference / (constants.seconds_per_minute*constants.minutes_per_hour)
      local percent_chance = (elapsed_hours / constants.hours_per_day) * 100
      --TODO: increase percent_chance based on shepherd level
      --percent_chance = percent_chance + self._sv.bonus
      local roll = rng:get_int(1, 100)  
      if roll < percent_chance then
         self._sv.last_found_critter_time = curr_elapsed_time
         return true
      end
   end
   return false
end

-- Private Functions

--Remove their tags and make sure they are free from their pasture
function ShepherdClass:_abandon_following_animals()
   if self._sv.trailed_animals then
      for id, animal in pairs(self._sv.trailed_animals) do
         local equipment_component = animal:get_component('stonehearth:equipment')
         local pasture_tag = equipment_component:has_item_type('stonehearth:pasture_tag')
         local shepherded_animal_component = pasture_tag:get_component('stonehearth:shepherded_animal')
         local pasture = shepherded_animal_component:get_pasture()
         if pasture then 
            local pasture_component = pasture:get_component('stonehearth:shepherd_pasture')
            pasture_component:remove_animal(id)
         end
         equipment_component:unequip_item('stonehearth:pasture_tag')
         self:remove_trailing_animal(id)
      end
      self._sv.trailed_animals = nil
   end
end

return ShepherdClass
