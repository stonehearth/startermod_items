local ShepherdPastureComponent = class()
local Point3 = _radiant.csg.Point3

function ShepherdPastureComponent:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()   

   self.default_type = json.default_type

   --On init or load, the timer is always nil
   self._reproduction_timer = nil

   if not self._sv.initialized then
      self._sv.initialized = true
      self._sv.pasture_data = json.pasture_data
      self._sv.tracked_critters = {}
      self._sv.num_critters = 0
      self._sv.reproduction_time_interval = nil
   else
      --If we're loading, we can just create the tasks
      self:_create_animal_collection_tasks()

      --also, if there is an outstanding reproduction timer, we should create that
      radiant.events.listen(radiant, 'radiant:game_loaded', function(e)
         self:_create_load_timer()
         return radiant.events.UNLISTEN
      end)
   end
end

-- Resumes the reproduction timer if it was going while we saved
function ShepherdPastureComponent:_create_load_timer()
   if self._sv._expiration_time then
      local duration = self._sv._expiration_time - stonehearth.calendar:get_elapsed_time()
      self._reproduction_timer = stonehearth.calendar:set_timer(duration, function() 
         self:_reproduce_animal()
      end)
   end
end

--On destroying the pasture, nuke the tasks
function ShepherdPastureComponent:destroy()
   self:_destroy_animal_collection_tasks()
end


function ShepherdPastureComponent:get_size()
   return self._sv.size
end

function ShepherdPastureComponent:set_size(x, z)
   self._sv.size = { x = x, z = z}
   local ten_square_units = x * z / 100
   --For each animal type in the pasture_data, go through and reset max_population
   for animal, data in pairs(self._sv.pasture_data) do
      data.max_population = math.floor(data.max_num_per_10_square_unit * ten_square_units)
   end

   self.__saved_variables:mark_changed()
end

-- Returns 
function ShepherdPastureComponent:get_center_point()
   local region_shape = self._entity:add_component('region_collision_shape'):get_region():get()
   local centroid = _radiant.csg.get_region_centroid(region_shape) --returns a point in the middle
   local location = radiant.entities.get_world_grid_location(self._entity)
   return Point3(location.x + centroid.x, location.y, location.z + centroid.z) --centroid
end

--This causes critters of the existing type to be released
--Fires the task telling the shepherd to go get more critters of that type
function ShepherdPastureComponent:set_pasture_type(new_animal_type)
   self:_release_existing_animals()
   if not new_animal_type then
      new_animal_type = self.default_type
   end
   self._sv.pasture_type = new_animal_type
   self:_create_animal_collection_tasks()
end

function ShepherdPastureComponent:get_pasture_type()
   return self._sv.pasture_type
end

--Returns the array of animals in the pasture
function ShepherdPastureComponent:get_animals()
   return self._sv.tracked_critters
end

function ShepherdPastureComponent:get_num_animals()
   return self._sv.num_critters
end

--Returns the minimum number of animals for this pasture
function ShepherdPastureComponent:get_min_animals()
   return self._sv.pasture_data[self._sv.pasture_type].min_population
end

--Returns the max number of animals in the pasture
function ShepherdPastureComponent:get_max_animals()
   return self._sv.pasture_data[self._sv.pasture_type].max_population
end

function ShepherdPastureComponent:add_animal(animal)
   self._sv.tracked_critters[animal:get_id()] = animal
   self._sv.num_critters = self._sv.num_critters + 1
   self:_calculate_reproduction_timer()
end

function ShepherdPastureComponent:remove_animal(animal_id)
   self._sv.tracked_critters[animal_id] = nil
   self._sv.num_critters = self._sv.num_critters - 1

   assert(self._sv.num_critters >= 0)
   self:_calculate_reproduction_timer()
end

--------- Private functions

function ShepherdPastureComponent:_release_existing_animals()
   --TODO: undo all their leashes or whatever
   self._sv.tracked_critters = {}
end

-- Destroy any existing animal collection tasks, and make some new ones
function ShepherdPastureComponent:_create_animal_collection_tasks()
   self:_destroy_animal_collection_tasks()

   local town = stonehearth.town:get_town(self._entity)
   self._animal_collection_task = town:create_task_for_group(
      'stonehearth:task_group:herding', 
      'stonehearth:collect_animals_for_pasture', 
      {pasture = self._entity})
      :set_source(self._entity)
      :set_name('collect animals for pasture')
      :set_priority(stonehearth.constants.priorities.shepherding_animals.GATHER_ANIMALS)
      :start()
end

function ShepherdPastureComponent:_destroy_animal_collection_tasks()
   if self._till_task then
      self._till_task = nil
      self._till_task:destroy()
   end
end

--Returns true if we can numerically reproduce, false otherwise
function ShepherdPastureComponent:_reproduction_check()
   --Can't reproduce if there's less than 2 critters in the pen
   if self._sv.num_critters < 2 or self._sv.num_critters > self:get_max_animals() then
      return false
   end
   return true
end

-- Calculate the reproduction period. If there is no timer, then start one
function ShepherdPastureComponent:_calculate_reproduction_timer()
   if not self:_reproduction_check() then
      self._sv.reproduction_time_interval = nil
      return
   end

   local interval = self._sv.pasture_data[self._sv.pasture_type].base_reproduction_period
   --add in whatever shepherd-related, animal-related scores here
   --the timer is variable, so we don't just use the interval timer in the growth service
   --this way the shepherd's level, etc can affect reproduction time
   --avoiding the issue we had trying to get the farmer to change the repro time on crops
   self._sv.reproduction_time_interval = interval

   --If we already have a reproduction timer, then don't start a new one
   --if we don't have an interval, don't start the timer
   if not self._reproduction_timer and self._sv.reproduction_time_interval then 
      self._reproduction_timer = stonehearth.calendar:set_timer(self._sv.reproduction_time_interval, function()
         self:_reproduce_animal()
      end)
      self._sv._expiration_time = self._reproduction_timer:get_expire_time()
   end
end

-- If there's less than 2 critters, no reproduction, stop conversations about widowed pregnant sheep right here
-- TODO: baby animals!
-- TODO: should the shepherd help deliver the animals? Too weird? Not applicable to like, chickens?
function ShepherdPastureComponent:_reproduce_animal()
   if not self:_reproduction_check() then
      return
   end

   --TODO: OMG, baby animals! We want baby animals!
   local animal = radiant.entities.create_entity(self._sv.pasture_type)
   local equipment_component = animal:add_component('stonehearth:equipment')
   local pasture_collar = radiant.entities.create_entity('stonehearth:pasture_tag')
   equipment_component:equip_item(pasture_collar)
   local shepherded_animal_component = pasture_collar:get_component('stonehearth:shepherded_animal')
   shepherded_animal_component:set_animal(animal)
   shepherded_animal_component:set_pasture(self._entity)
   self:add_animal(animal)
   radiant.terrain.place_entity(animal, self:get_center_point())

   --TODO: replace this effect with a cooler one
   radiant.effects.run_effect(animal, '/stonehearth/data/effects/fursplosion_effect/growing_wool_effect.json')

   --Nuke the timer, we're done with it
   self._reproduction_timer = nil
   self._sv._expiration_time = nil

   --start the timer again
   self:_calculate_reproduction_timer()
end

return ShepherdPastureComponent