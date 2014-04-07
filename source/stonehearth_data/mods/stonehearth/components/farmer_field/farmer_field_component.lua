--[[
   Stores data about the field in question
]]

local farming_service = stonehearth.farming

local FarmerFieldComponent = class()
FarmerFieldComponent.__classname = 'FarmerFieldComponent'

local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local rng = _radiant.csg.get_default_rng()

function FarmerFieldComponent:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()   
   self.__saved_variables:set_controller(self)

   self._till_tasks = {}

   if not self._sv._initialized then
      -- creating for the 1st time...
      self._sv._initialized = true
      self._sv.size = {0, 0}
      self._sv.location = nil
      self._sv.contents = {}

      local min_fertility = stonehearth.constants.soil_fertility.MIN
      local max_fertility = stonehearth.constants.soil_fertility.MAX
      self._sv.general_fertility = rng:get_int(min_fertility, max_fertility)   --TODO; get from global service
      
      self._sv.crop_queue = {farming_service:get_crop_details('fallow')}
      --self._sv.crop_queue = {}
      self._sv.curr_crop = 1
      self._sv.auto_harvest = true
      self._sv.auto_replant = true
   else 
      -- loading, wait for everything to be loaded to create the tasks
      radiant.events.listen(radiant, 'radiant:game_loaded', function(e)
            self:_re_init_field()
            return radiant.events.UNLISTEN
         end)
   end
   --self.__saved_variables:mark_changed()
   --TODO: listen on changes to faction, like stockpile?
end

--- Recreate the currently running tasks from the existing state
--  TODO: how to hang onto the user-generated tasks if the farm isn't present?
function FarmerFieldComponent:_re_init_field()
   local town = stonehearth.town:get_town(self._entity)
   for x=1, self._sv.size[1] do
      self._till_tasks[x] = {}
      for y=1, self._sv.size[2] do
         local field_spacer = self._sv.contents[x][y].plot

         --Listen for when stuff happens on this field
         radiant.events.listen(field_spacer, 'stonehearth:crop_removed', self, self._on_crop_removed)

         --Has this spot been tilled yet? If not, retill 
         if not self._sv.contents[x][y].till_task_complete then
            --We still have to till this part of the field
            self._till_tasks[x][y] = town:create_farmer_task('stonehearth:till_field', { field_spacer = field_spacer, field = self })
                                    :set_source(field_spacer)
                                    :set_name('till field task')
                                    :set_priority(stonehearth.constants.priorities.farmer_task.TILL)
                                    :once()
                                    :start()
         end

         --If this spot is empty of contents, determine if we should replant something
         local dirt_plot_component = field_spacer:get_component('stonehearth:dirt_plot')
         local plant = dirt_plot_component:get_contents()
         if not plant then
            local e = {}
            e.plot_entity = field_spacer
            self:_determine_replant(e)
         else
            --If a plant is harvestable, it should handle the harvest itself.
            --there is a plant! Should we harvest it yet?
            --local crop_component = plant:get_component('stonehearth:crop')
            --if crop_component:is_harvestable() then
            --   self:determine_auto_harvest(dirt_plot_component, plant)
            --end
         end

      end
   end
end

--- On destroy, remove all listeners from the plots
function FarmerFieldComponent:destroy()
   --Unlisten on all the field plot things
   for x=1, self._sv.size[1] do
      for y=1, self._sv.size[2] do
         local field_spacer = self._sv.contents[x][y].plot
         if field_spacer then
            local dirt_plot_component = field_spacer:get_component('stonehearth:dirt_plot')
            if dirt_plot_component then
               dirt_plot_component:set_field(nil, nil)
            end
            radiant.events.unlisten(field_spacer, 'stonehearth:crop_removed', self, self._on_crop_removed)
         end
         local till_task =  self._till_tasks[x][y]
         if till_task then
            till_task:destroy()
            till_task = nil
         end
      end
   end
end

--TODO: Depending on how we eventually designate whether fields can overlap (no?)
--consider moving this into a central service
function FarmerFieldComponent:create_dirt_plots(town, location, size)
   self._sv.size = { size[1], size[2] }
   self._sv.location = location

   for x=1, self._sv.size[1] do
      self._sv.contents[x] = {}
      self._till_tasks[x] = {}
      for y=1, self._sv.size[2] do
         --init the dirt plot
         local field_spacer = self:_init_dirt_plot(location, x, y)
         self._sv.contents[x][y] = {}
         self._sv.contents[x][y].plot = field_spacer

         -- Tell the farmer scheduler to till this
         self._sv.contents[x][y].till_task_complete = false
         self._till_tasks[x][y] = town:create_farmer_task('stonehearth:till_field', { field_spacer = field_spacer, field = self })
                                   :set_source(field_spacer)
                                   :set_name('till field task')
                                   :set_priority(stonehearth.constants.priorities.farmer_task.TILL)
                                   :once()
                                   :start()
      end
   end
   self.__saved_variables:mark_changed()
end

--- Init the dirt plot's model variant, and place it in the world.
--  The dirt plot starts invisible, and then once tilled, appears.
function FarmerFieldComponent:_init_dirt_plot(location, x, y)
   local field_spacer = radiant.entities.create_entity('stonehearth:tilled_dirt') 
   local render_info = field_spacer:add_component('render_info')
   render_info:set_model_variant('untilled_ground')
   local dirt_plot_component = field_spacer:get_component('stonehearth:dirt_plot')
   dirt_plot_component:set_field(self._entity, {x=x, y=y})

   -- every even column in the farm is a furrow
   if x % 2 == 0 then
      dirt_plot_component:set_furrow(true)
   end

   local grid_location = Point3(location.x + x-1, 0, location.z + y-1)
   radiant.terrain.place_entity(field_spacer, grid_location)

   --Listen for events on the plot
   radiant.events.listen(field_spacer, 'stonehearth:crop_removed', self, self._on_crop_removed)

   return field_spacer
end

--[[
--Fill ths in when we have a full queue implementation
--TODO: figure out the exact interaction between the UI and the queue update mechanism
function FarmerFieldComponent:update_queue(session, response, updated_queue, curr_crop_index)
   self._sv.crop_queue = updated_queue
   self._sv.curr_crop = curr_crop_index
   self.__saved_variables:mark_changed()
   return true
end
--]]

--Temporary: replaces queue item with a brand new item
function FarmerFieldComponent:change_default_crop(session, response, new_crop)
   self._sv.crop_queue = {farming_service:get_crop_details(new_crop)}
   self:_re_evaluate_empty()
   self.__saved_variables:mark_changed()
   return true
end

--Iterate through the field. If there is an empty space, determine if we should re-plant there
function FarmerFieldComponent:_re_evaluate_empty()
   for x=1, self._sv.size[1] do
      for y=1, self._sv.size[2] do
         local field_spacer = self._sv.contents[x][y].plot
         local dirt_plot_component = field_spacer:get_component('stonehearth:dirt_plot')
         if not dirt_plot_component:get_contents() then
            local e = {}
            e.plot_entity = field_spacer
            self:_determine_replant(e)
         end
      end
   end
end

--- On crop remove, figure out if we should auto-replant the last-planted
function FarmerFieldComponent:_on_crop_removed(e)
   self:_determine_replant(e)
end

--- Given the field and dirt data, replant the last crop
--  Always follow the policy set on the plot, if anything
--  If nothing is set on the plot, follow the policy on the field
--  If the next plant is nil, then don't plant anything. 
function FarmerFieldComponent:_determine_replant(e)
   --Figure out the policy on the field
   local plot_entity = e.plot_entity
   local dirt_component = plot_entity:get_component('stonehearth:dirt_plot')

   -- furrows are never planted
   if dirt_component:is_furrow() then
      return
   end

   local do_replant = self._sv.auto_replant
   local do_auto_harvest = self._sv.auto_harvest
   local next_plant = self:_get_next_queued_crop()

   local player_override = dirt_component:get_player_override()

   --Override with data from the plot, if applicable
   if dirt_component:get_player_override() then
      do_replant = dirt_component:get_replant()
      do_auto_harvest = dirt_component:get_auto_harvest()
      next_plant = dirt_component:get_last_planted_type()
   end

   --If we're supposed to replant, and if we have a plant to replant, do replant
   if do_replant and next_plant then
      local field_location = e.location
      farming_service:plant_crop(radiant.entities.get_player_id(self._entity), {plot_entity}, next_plant, player_override, do_replant, do_auto_harvest, false)
   end
end

--- Determine the crop that should be planted next, according the queue on this field
--  If the crop type is "fallow" the uri will be nil, and willr eturn nil
--  TODO: listen on an event to determine when to roll the crops over to the next period
function FarmerFieldComponent:_get_next_queued_crop()
   local next_plant = self._sv.crop_queue[self._sv.curr_crop]
   if next_plant then
      next_plant = next_plant.uri
   end
   return next_plant
end

--- Given the field and dirt data, harvest the crop
function FarmerFieldComponent:determine_auto_harvest(dirt_component, crop)
   local do_harvest = self._sv.auto_harvest
   if dirt_component:get_player_override() then
      do_harvest = dirt_component:get_auto_harvest()
   end
   if do_harvest then
      local town = stonehearth.town:get_town(self._entity)
      return town:harvest_resource_node(crop)
   end
end

--- Call when it's time to till the ground for the first time
--  TODO: model varients for the dirt
function FarmerFieldComponent:till_location(field_spacer)
   --TODO: get general fertility data from a global service
   --and maybe local fertility data too
   local local_fertility = rng:get_gaussian(self._sv.general_fertility, stonehearth.constants.soil_fertility.VARIATION)
   local dirt_plot_component = field_spacer:get_component('stonehearth:dirt_plot')

   --TODO: set moisture correctly
   dirt_plot_component:set_fertility_moisture(local_fertility, 50)

   --TODO: mark tilled task complete in the index
   local location = dirt_plot_component:get_location()
   self._sv.contents[location.x][location.y].till_task_complete = true

   --Check if we should automatically plant something here
   --(TODO: for now, initial planting handled by the UI)
   --local e = {}
   --e.plot_entity = field_spacer
   --self:_determine_replant(e)
end


return FarmerFieldComponent