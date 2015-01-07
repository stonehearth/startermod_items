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
   self:_create_xp_listeners()

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
   self:_remove_xp_listeners()
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

   --If we have the shepherd_speed_buff, make sure the added critter gets the buff too
   if self:has_perk('shepherd_speed_up_1') then
      radiant.entities.add_buff(animal, 'stonehearth:buffs:shepherd:speed_1');
   end

   --Fire an event saying that we've collected an animal
   radiant.events.trigger(self._sv._entity, 'stonehearth:add_trailing_animal', {animal = animal, pasture = pasture})
end

function ShepherdClass:get_trailing_animals()
   return self._sv.trailed_animals, self._sv.num_trailed_animals
end

--Remove the animal from the shepherd's list. 
function ShepherdClass:remove_trailing_animal(animal_id)
   if not self._sv.trailed_animals then
      return
   end

   --If the animal had the speed buff, remove it
   if self._sv.trailed_animals[animal_id] and self:has_perk('shepherd_speed_up_1') then
      radiant.entities.remove_buff(self._sv.trailed_animals[animal_id], 'stonehearth:buffs:shepherd:speed_1')
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
      if self:has_perk('shepherd_improved_find_rate') then
         percent_chance = percent_chance * 2
      end
      local roll = rng:get_int(1, 100)  
      if roll < percent_chance then
         self._sv.last_found_critter_time = curr_elapsed_time
         return true
      end
   end
   return false
end

-- Private Functions
function ShepherdClass:_create_xp_listeners()
   self._find_animal_listener = radiant.events.listen(self._sv._entity, 'stonehearth:tame_animal', self, self._on_animal_tamed)
   self._harvest_renwable_resources_listener = radiant.events.listen(self._sv._entity, 'stonehearth:gather_renewable_resource', self, self._on_renewable_resource_gathered)
end

function ShepherdClass:_remove_xp_listeners()
   self._find_animal_listener:destroy()
   self._find_animal_listener = nil

   self._harvest_renwable_resources_listener:destroy()
   self._harvest_renwable_resources_listener = nil
end

-- When we tame an animal, grant some XP
-- TODO: maybe vary the XP based on the kind of animal?
function ShepherdClass:_on_animal_tamed(args)
   self._job_component:add_exp(self._sv.xp_rewards['tame_animal'])
end

--Grant some XP if we harvest renwable resources off an animal
function ShepherdClass:_on_renewable_resource_gathered(args)
   if args.harvested_target then
      local equipment_component = args.harvested_target:get_component('stonehearth:equipment')
      if equipment_component and equipment_component:has_item_type('stonehearth:pasture_tag') then
         self._job_component:add_exp(self._sv.xp_rewards['harvest_animal_resources'])
      end
   end
   if args.spawned_item and self:has_perk('shepherd_extra_bonuses') then
      local spawned_uri = args.spawned_item:get_uri()
      local source_location = radiant.entities.get_world_grid_location(self._sv._entity)
      local placement_point = radiant.terrain.find_placement_point(source_location, 1, 5)
      if not placement_point then
         placement_point = source_location
      end
      local extra_harvest = radiant.entities.create_entity(spawned_uri)
      radiant.terrain.place_entity(extra_harvest, placement_point)
   end
end

--Remove their tags and make sure they are free from their pasture
function ShepherdClass:_abandon_following_animals()
   if self._sv.trailed_animals then
      for id, animal in pairs(self._sv.trailed_animals) do
         --Free self from pasture and tag
         local equipment_component = animal:get_component('stonehearth:equipment')
         local pasture_tag = equipment_component:has_item_type('stonehearth:pasture_tag')
         local shepherded_animal_component = pasture_tag:get_component('stonehearth:shepherded_animal')
         shepherded_animal_component:free_animal()
      end
      self._sv.trailed_animals = nil
   end
end

-- apply the buff. If the buff is one that applies to all trailing animals too, apply it
function ShepherdClass:apply_buff(args)
   radiant.entities.add_buff(self._sv._entity, args.buff_name)

   if self:has_perk('shepherd_speed_up_1') then
      if self._sv.trailed_animals then
         for id, animal in pairs(self._sv.trailed_animals) do
            --Free self from pasture and tag
            radiant.entities.add_buff(animal, 'stonehearth:buffs:shepherd:speed_1')
         end
      end
   end
end

function ShepherdClass:remove_buff(args)
   radiant.entities.remove_buff(self._sv._entity, args.buff_name)
end


return ShepherdClass
