local ShepherdPastureComponent = class()
local Point3 = _radiant.csg.Point3

function ShepherdPastureComponent:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()   

   self.default_type = json.default_type

   if not self._sv.initialized then
      self._sv.initialized = true
      self._sv.pasture_data = json.pasture_data
      self._sv.tracked_critters = {}
      self._sv.num_critters = 0
      self._sv.reproduction_time_interval = nil
      self._sv.check_for_strays_interval = json.check_for_strays_interval
   else
      --If we're loading, we can just create the tasks
      
      --also, if there is an outstanding reproduction timer, we should create that
      radiant.events.listen_once(radiant, 'radiant:game_loaded', function(e)
         self:_create_animal_collection_tasks()
         self:_create_animal_listeners()
         self:_load_timers()
      end)
   end
end

-- Resumes timers if they were going while we saved
function ShepherdPastureComponent:_load_timers()
   if self._sv.reproduction_timer then
      self._sv.reproduction_timer:bind(function() 
         self:_reproduce_animal()
      end)
   end
   if self._sv.stray_interval_timer then
      self._sv.stray_interval_timer:bind(function()
            self:_collect_strays()
         end)
   end
end

--On destroying the pasture, nuke the tasks
--Release all the animals
function ShepherdPastureComponent:destroy()
   self:_destroy_animal_collection_tasks()

   if self._sv.stray_interval_timer then
      self._sv.stray_interval_timer:destroy()
      self._sv.stray_interval_timer = nil
   end

   if self._sv.reproduction_timer then
      self._sv.reproduction_timer:destroy()
      self._sv.reproduction_timer = nil
   end

  self:_release_existing_animals()
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
   self:_create_stray_timer()
   self.__saved_variables:mark_changed()
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
   self._sv.tracked_critters[animal:get_id()] = {entity = animal}
   self._sv.num_critters = self._sv.num_critters + 1
   self:_calculate_reproduction_timer()
   self:_listen_for_renewables(animal)
   self:_create_harvest_task(animal)

   self.__saved_variables:mark_changed()
   radiant.events.trigger(self._entity, 'stonehearth:on_pasture_animals_changed', {})
end


--------- Private functions

--When loading, refresh all the listeners on the animals
function ShepherdPastureComponent:_create_animal_listeners()
   for id, data in pairs(self._sv.tracked_critters) do 
      if data and data.entity then
         self:_listen_for_renewables(data.entity)
      end
   end
end

function ShepherdPastureComponent:_listen_for_renewables(animal)
   local event = radiant.events.listen(animal, 'stonehearth:on_renewable_resource_renewed', self, self._on_animal_renewable_renewed)
   self._sv.tracked_critters[animal:get_id()].renew_event = event
   self.__saved_variables:mark_changed()
end

--When a renewable has come back, send the shepherds to go farm it
function ShepherdPastureComponent:_on_animal_renewable_renewed(args)
   self:_create_harvest_task(args.target)
end

--Create a one-time harvest task for the renewable resource
function ShepherdPastureComponent:_create_harvest_task(target)
   local renewable_resource_component = target:get_component('stonehearth:renewable_resource_node')
   if renewable_resource_component and renewable_resource_component:is_harvestable() then
      local town = stonehearth.town:get_town(self._entity)
      town:create_task_for_group(
         'stonehearth:task_group:herding', 
         'stonehearth:harvest_plant',
         {plant = target})
         :set_source(target)
         :set_name('harvesting')
         :once()
         :set_priority(stonehearth.constants.priorities.shepherding_animals.HARVEST)
         :start()
   end
end

--If the pasture is deleted, we need to go through all the animals and remove their
--ties to the shepherd and their pasture tags.
function ShepherdPastureComponent:_release_existing_animals()
   for id, animal_data in pairs(self._sv.tracked_critters) do
      --If the animal is following a shepherd, remove it from the shepherd's folloq queue
      local animal = animal_data.entity
      local equipment_component = animal:add_component('stonehearth:equipment')
      if equipment_component then
         local pasture_tag = equipment_component:has_item_type('stonehearth:pasture_tag')
         if pasture_tag then
            local shepherded_animal_component = pasture_tag:get_component('stonehearth:shepherded_animal')
            shepherded_animal_component:free_animal()
         end
      end       
   end
   self._sv.tracked_critters = {}
   self.__saved_variables:mark_changed()
end

--Removes the animal from the pasture, but only once
function ShepherdPastureComponent:remove_animal(animal_id)
   --remove events associated with it
   local renew_event = self._sv.tracked_critters[animal_id].renew_event
   if renew_event then
      renew_event:destroy()
      renew_event = nil
   end

   if self._sv.tracked_critters[animal_id] then
      self._sv.tracked_critters[animal_id] = nil
      self._sv.num_critters = self._sv.num_critters - 1

      assert(self._sv.num_critters >= 0)
      self:_calculate_reproduction_timer()

      self.__saved_variables:mark_changed()
      radiant.events.trigger(self._entity, 'stonehearth:on_pasture_animals_changed', {})
   end
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
   if self._animal_collection_task then
      self._animal_collection_task:destroy()
      self._animal_collection_task = nil
   end
end

--Given the interval, find the strays.
function ShepherdPastureComponent:_create_stray_timer()
   if self._sv.stray_interval_timer then
      return
   end
   self._sv.stray_interval_timer = stonehearth.calendar:set_interval(self._sv.check_for_strays_interval, function()
      self:_collect_strays()
   end)
end

-- Iterate through the critters in the pasture.
-- If they are out of bounds AND not currently following a shepherd
-- then fire off a task to bring them home
function ShepherdPastureComponent:_collect_strays()
   for id, critter_data in pairs(self._sv.tracked_critters) do
      local critter = critter_data.entity

      if critter and critter:is_valid() then
         local critter_location = radiant.entities.get_world_grid_location(critter)
         local region_shape = self._entity:add_component('region_collision_shape'):get_region():get()
         
         local pasture_location = radiant.entities.get_world_grid_location(self._entity)
         local world_region_shape = region_shape:translated(pasture_location)

         local equipment_component = critter:get_component('stonehearth:equipment')
         local pasture_collar = equipment_component:has_item_type('stonehearth:pasture_tag')
         local shepherded_animal_component = pasture_collar:get_component('stonehearth:shepherded_animal')

         if not world_region_shape:contains(critter_location) and not shepherded_animal_component:get_following() then
            local town = stonehearth.town:get_town(self._entity)
            town:create_task_for_group(
               'stonehearth:task_group:herding', 
               'stonehearth:find_stray_animal', 
               {animal = critter, pasture = self._entity})
               :set_source(critter)
               :set_name('return strays to pasture')
               :set_priority(stonehearth.constants.priorities.shepherding_animals.RETURN_TO_PASTURE)
               :once()
               :start()
         end
      end
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
   if not self._sv.reproduction_timer and self._sv.reproduction_time_interval then 
      self._sv.reproduction_timer = stonehearth.calendar:set_timer(self._sv.reproduction_time_interval, function()
         self:_reproduce_animal()
      end)
   end
   self.__saved_variables:mark_changed()
end

-- If there's less than 2 critters, no reproduction, stop conversations about widowed pregnant sheep right here
-- TODO: baby animals!
-- TODO: should the shepherd help deliver the animals? Too weird? Not applicable to like, chickens?
function ShepherdPastureComponent:_reproduce_animal()
   if not self:_reproduction_check() then
      return
   end

   --TODO: OMG, baby animals! We want baby animals!
   local animal = radiant.entities.create_entity(self._sv.pasture_type, { owner = self._entity })
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
   if self._sv.reproduction_timer then
      self._sv.reproduction_timer:destroy()
      self._sv.reproduction_timer = nil
   end
   --start the timer again
   self:_calculate_reproduction_timer()
   self.__saved_variables:mark_changed()
end

return ShepherdPastureComponent