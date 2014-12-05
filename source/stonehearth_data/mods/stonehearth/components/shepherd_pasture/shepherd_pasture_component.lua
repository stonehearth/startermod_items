local ShepherdPastureComponent = class()
local Point3 = _radiant.csg.Point3

function ShepherdPastureComponent:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()   

   self.default_type = json.default_type

   if not self._sv.initialized then
      self._sv.initialized = true
      self._sv.pasture_data = json.pasture_data

   else

      --If we're loading, we can just create the tasks
      self:_create_animal_collection_tasks()
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

--Returns the minimum number of animals for this pasture
function ShepherdPastureComponent:get_min_animals()
   return self._sv.pasture_data[self._sv.pasture_type].min_population
end

--Returns the max number of animals in the pasture
function ShepherdPastureComponent:get_max_animals()
   return self._sv.pasture_data[self._sv.pasture_type].max_population
end

function ShepherdPastureComponent:add_animal(animal)
   table.insert(self._sv.tracked_critters, animal)
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



return ShepherdPastureComponent