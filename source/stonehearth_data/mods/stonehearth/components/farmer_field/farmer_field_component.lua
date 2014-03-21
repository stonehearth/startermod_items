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

   --TODO: add some of these things to CREATE so they can be loaded properly
   if not self._sv._initialized then
      self._sv._initialized = true
      self._sv.size = {0, 0}
      self._sv.location = nil
      self._sv.contents = {}
      self._sv.general_fertility = rng:get_int(1, 40)   --TODO; get from global service
      self._sv.crop_queue = {farming_service:get_crop_details('fallow')}
      --self._sv.crop_queue = {}
      self._sv.curr_crop = 1
      self._sv.auto_harvest = true
      self._sv.auto_replant = true
   end
   --self.__saved_variables:mark_changed()
   --TODO: listen on changes to faction, like stockpile?
end

--- On destroy, remove all listeners from the plots
--  TODO: remove tasks also?
function FarmerFieldComponent:destroy()
   --Unlisten on all the field plot things
   for x=1, self._sv.size[1] do
      for y=1, self._sv.size[2] do
         local field_spacer = self._sv.contents[x][y].plot
         local dirt_plot_component = field_spacer:get_component('stonehearth:dirt_plot')
         dirt_plot_component:set_field(nil, nil)
         radiant.events.unlisten(field_spacer, 'stonehearth:crop_removed', self, self._on_crop_removed)
         local till_task =  self._sv.contents[x][y].till_task
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
      for y=1, self._sv.size[2] do
         --init the dirt plot
         local field_spacer = self:_init_dirt_plot(location, x, y)
         self._sv.contents[x][y] = {}
         self._sv.contents[x][y].plot = field_spacer

         -- Tell the farmer scheduler to till this
         self._sv.contents[x][y].till_task = town:create_farmer_task('stonehearth:till_field', { field_spacer = field_spacer, field = self })
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
function FarmerFieldComponent:_init_dirt_plot(location, x, y)
   local field_spacer = radiant.entities.create_entity('stonehearth:tilled_dirt') 
   local render_info = field_spacer:add_component('render_info')
   render_info:set_model_variant('untilled_ground')
   local dirt_plot_component = field_spacer:get_component('stonehearth:dirt_plot')
   dirt_plot_component:set_field(self, {x=x, y=y})

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
--  Always do whatever is set on the plot, if anything
--  If nothing is set on the plot, follow the policy on the field
--  If the next plant is nil, then don't plant anything. 
function FarmerFieldComponent:_determine_replant(e)
   --Figure out the policy on the field
   local plot_entity = e.plot_entity
   local dirt_component = plot_entity:get_component('stonehearth:dirt_plot')
   local do_replant = self._sv.auto_replant
   local next_plant = self:_get_next_queued_crop()

   --Override with data from the plot, if applicable
   if dirt_component:get_player_override() then
      do_replant = dirt_component:get_replant()
      next_plant = dirt_component:get_last_planted_type()
   end

   --If we're supposed to replant, and if we have a plant to replant, do replant
   if do_replant and next_plant then
      local field_location = e.location
      farming_service:plant_crop(radiant.entities.get_player_id(self._entity), {plot_entity}, next_plant)
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
   local local_fertility = rng:get_gaussian(self._sv.general_fertility, 10)
   local dirt_plot_component = field_spacer:get_component('stonehearth:dirt_plot')

   --TODO: set moisture correctly
   dirt_plot_component:set_fertility_moisture(local_fertility, 50)

   --Check if we should automatically plant something here
   local e = {}
   e.plot_entity = field_spacer
   self:_determine_replant(e)
end


return FarmerFieldComponent