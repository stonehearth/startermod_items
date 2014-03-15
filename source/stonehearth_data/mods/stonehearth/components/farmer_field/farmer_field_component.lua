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

   --TODO: add some of these things to CREATE so they can be loaded properly
   self._data = {
      size = {0, 0}, 
      location = nil,
      contents = {},
      general_fertility = rng:get_int(1, 40),   --TODO; get from global service
      auto_harvest = true, 
      auto_replant = true
   }

   self.__saved_variables = radiant.create_datastore(self._data)
   self.__saved_variables:mark_changed()
   --TODO: listen on changes to faction, like stockpile?
end

--- On destroy, remove all listeners from the plots
--  TODO: remove tasks also?
function FarmerFieldComponent:__destroy()
   --Unlisten on all the field plot things
   for x=1, self._data.size[1] do
      self._data.contents[x] = {}
      for y=1, self._data.size[2] do
         radiant.events.unlisten(field_spacer, 'stonehearth:crop_removed', self, self._on_crop_removed)
      end
   end
end

--TODO: Depending on how we eventually designate whether fields can overlap (no?)
--consider moving this into a central service
function FarmerFieldComponent:create_dirt_plots(town, location, size)
   self._data.size = { size[1], size[2] }
   self._data.location = location

   for x=1, self._data.size[1] do
      self._data.contents[x] = {}
      for y=1, self._data.size[2] do
         --init the dirt plot
         local field_spacer = self:_init_dirt_plot(location, x, y)
         self._data.contents[x][y] = field_spacer

         -- Tell the farmer scheduler to till this
         town:create_farmer_task('stonehearth:till_field', { field_spacer = field_spacer, field = self })
                                   :set_source(field_spacer)
                                   :set_name('till field task')
                                   :once()
                                   :start()
      end
   end

   --TODO: where to schedule tasks for planting and harvesting and destroying things?
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

--- On crop remove, figure out if we should auto-replant the last-planted
function FarmerFieldComponent:_on_crop_removed(e)
   self:_determine_replant(e)
end

--- Given the field and dirt data, replant the last crop
--  Always do whatever is set on the plot, if anything
--  If nothing is set on the plot, follow the policy on the field
function FarmerFieldComponent:_determine_replant(e)
   local plot_entity = e.plot_entity
   local dirt_component = plot_entity:get_component('stonehearth:dirt_plot')
   local do_replant = self._data.auto_replant

   if dirt_component:get_player_override() then
      do_replant = dirt_component:get_replant()
   end

   if do_replant then
      local field_location = e.location
      local last_type = dirt_component:get_last_planted_type()
      farming_service:plant_crop(radiant.entities.get_player_id(self._entity), {plot_entity}, last_type)
   end
end

--- Given the field and dirt data, harvest the crop
function FarmerFieldComponent:determine_auto_harvest(dirt_component, crop)
   local do_harvest = self._data.auto_harvest
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
   local local_fertility = rng:get_gaussian(self._data.general_fertility, 10)
   local dirt_plot_component = field_spacer:get_component('stonehearth:dirt_plot')

   --TODO: set moisture correctly
   dirt_plot_component:set_fertility_moisture(local_fertility, 50)
end


return FarmerFieldComponent