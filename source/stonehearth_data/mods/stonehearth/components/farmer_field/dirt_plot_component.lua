local Point3 = _radiant.csg.Point3
local farming_service = stonehearth.farming

local DirtPlotComponent = class()

function DirtPlotComponent:initialize(entity, json)
   self._entity = entity
   self._data = {
      fertility = 0,
      moisture = 0,
      fertility_category = nil,
      parent_field = nil,
      field_location = nil,
      contents = nil
   }
   self.__saved_variables = radiant.create_datastore(self._data)
   self.__saved_variables:mark_changed()

   --listen for when something gets put on me
   --radiant.events.listen(self._entity, 'stonehearth:crop_planted', self, self._on_crop_planted)
end

--- Set the field and location in field for this plot
--  @param field: the field entity that we care about
--  @param location: the x/y coordinates of the plot. location[1] is x, location[2] is y
function DirtPlotComponent:set_field(parent_field, location)
   self._data.parent_field = parent_field
   self._data.field_location = location
   self.__saved_variables:mark_changed()
end

--- Given data about the plot, set the right state variables
--  @param fertility: a number between 0 and 40
--  @param moisture: a number between 0 and 100
function DirtPlotComponent:set_fertility_moisture(fertility, moisture)
   self._data.fertility = fertility
   self._data.moisture = moisture
   self:_update_visible_soil_state()
   self.__saved_variables:mark_changed()
end

function DirtPlotComponent:get_contents()
   return self._data.contents
end

--TODO: incorporate the moisture of the soil
function DirtPlotComponent:_update_visible_soil_state()
   local fertility_category = nil
   if  self._data.fertility < 10 then
      fertility_category = 'dirt_1'
   elseif  self._data.fertility < 25 then
      fertility_category = 'dirt_2'
   elseif  self._data.fertility < 35 then
      fertility_category = 'dirt_3'
   else 
      fertility_category = 'dirt_4'
   end

   --If the category has changed, propagate the change through all
   --visible elements
   if fertility_category ~= self._data.fertility_category then
      self._data.fertility_category = fertility_category

      local render_info = self._entity:add_component('render_info')
      render_info:set_model_variant(fertility_category)
      local dirt_name, dirt_description = farming_service:get_dirt_descriptions(fertility_category)
      radiant.entities.set_name(self._entity, dirt_name)
      radiant.entities.set_description(self._entity, dirt_description)
   end
end

--- Note: Plots are assumed to start empty. If you programatically put a plant down
--- in a crop, be sure to start with this function. 

--- Called when something is planted on me
--  @param e.crop = new crop planted
function DirtPlotComponent:plant_crop(crop_type)
   --Assert that there's nothing here 
   --TODO: use tasks to make sure things aren't planted twice
   --assert(self._data.contents == nil, "error, trying to plant on an occupied square")
   if self._data.contents ~= nil then
      return
   end

   --Unlisten on the event
   --radiant.events.unlisten(self._entity, 'stonehearth:crop_planted', self, self._on_crop_planted)

   local planted_entity = radiant.entities.create_entity(crop_type)
   radiant.terrain.place_entity(planted_entity, radiant.entities.get_world_grid_location(self._entity))
   self._data.contents = planted_entity

   --If the planted entity is a crop, add a reference to the dirt it sits on. 

   --Hide the plant command, add the raze command
   local command_component = self._entity:add_component('stonehearth:commands')
   --TODO: programatically remove all plant commands
   --command_component:remove_command('plant_crop')
   command_component:remove_command('plant_corn')
   command_component:remove_command('plant_turnip')

   command_component:add_command('/stonehearth/data/commands/raze_crop')

   --listen for when the crop is grown, and swapped for its final phase
   --radiant.events.listen(self._data.contents, 'stonehearth:crop_swap', self, self._on_crop_swapped)

   --listen for if the planted crop gets destroyed for any reason
   radiant.events.listen(planted_entity, 'stonehearth:entity:pre_destroy', self, self._on_crop_removed)

end

--- Called when something is removed from me
function DirtPlotComponent:_on_crop_removed()
   --Assert that there's something here 
   --TODO: use tasks to only allow things to be planted/harvested once
   --assert(self._data.contents ~= nil, "error, removing a crop that isn't there")
   if self._data.contents == nil then
      return
   end

   --Hide the raze command, add the plant command
   local command_component = self._entity:add_component('stonehearth:commands')
   command_component:remove_command('raze_crop')
   --TODO: programatically add all plant commands from farm
   --command_component:add_command('/stonehearth/data/commands/plant_crop')
   command_component:add_command('/stonehearth/data/commands/plant_crop/plant_turnip.json')
   command_component:add_command('/stonehearth/data/commands/plant_crop/plant_corn.json')

   self._data.contents = nil

   return radiant.events.UNLISTEN
end


return DirtPlotComponent