FarmingService = class()

function FarmingService:__init()
   self:_load_dirt_descriptions()
   self._crop_overlays = {}
end

--- Track the crops currently available to each community
--  Start the town with a certain number of crops
function FarmingService:initialize()
   self._data = {
      town_crops = {}
   }
   self.__saved_variables = radiant.create_datastore(self._data)
   self.__saved_variables:mark_changed()
end

function FarmingService:restore(saved_variables)
end

function FarmingService:_load_dirt_descriptions()
   local dirt_data = radiant.resources.load_json('stonehearth:tilled_dirt')
   self._dirt_data = {}
   self._dirt_data['dirt_1'] = dirt_data.components.model_variants['dirt_1']
   self._dirt_data['dirt_2'] = dirt_data.components.model_variants['dirt_2']
   self._dirt_data['dirt_3'] = dirt_data.components.model_variants['dirt_3']
   self._dirt_data['dirt_4'] = dirt_data.components.model_variants['dirt_4']
end

--TODO: revisit when we gate farmables by seeds that people start with
function FarmingService:_load_initial_crops()
   self._initial_crops = radiant.resources.load_json('farmer:initial_crops')
end

function FarmingService:get_dirt_descriptions(dirt_type)
   local dirt_details = self._dirt_data[dirt_type]
   return dirt_details.unit_info_name, dirt_details.unit_info_description
end

--- Get the crop types available to a faction. Start with a couple if there are none so far.
function FarmingService:get_all_crop_types(faction)
   return self:_get_crop_list(faction)
end

--- Add a new crop type to the faction
function FarmingService:add_crop_type(faction, new_crop_uri)
   local crop_list = self:_get_crop_list(faction)
   table.insert(crop_list, new_crop_uri)
   return crop_list
end

function FarmingService:_get_crop_list(faction)
   local crop_list = self._data.town_crops[faction]
   if not crop_list then
      --TODO: where is the kingdom name from? Originally, I'd put it into unit_info,
      --but Tony took it out again. We need to keep some "game variables" and this is
      --definitely one of them
      crop_list = self._initial_crops['ascendancy']
      self._data.town_crops[faction] = crop_list
   end
   return crop_list
end

--- Tell farmers to plan the crop_type in the designated locations
-- @param faction: the group that should be planting the crop
-- @param soil_plots: array of entities on top of which to plant the crop
-- @param crop_type: the name of the thing to plant (ie, stonehearth:corn, etc)
function FarmingService:plant_crop(faction, soil_plots, crop_type)
   if not soil_plots[1] or not crop_type then
      return false
   end
   local town = stonehearth.town:get_town(faction)
   for i, plot in ipairs(soil_plots) do
      --TODO: store these tasks, so they can be cancelled
      local overlay_effect = self:_get_overlay_for_crop(crop_type)
      town:create_farmer_task('stonehearth:plant_crop', {target_plot = plot, 
                                                         dirt_plot_component = plot:get_component('stonehearth:dirt_plot'),
                                                         crop_type = crop_type})
                              :set_source(plot)
                              :add_entity_effect(plot, overlay_effect)
                              :set_name('plant_crop')
                              :once()
                              :start()
   end
   return true
end

function FarmingService:_get_overlay_for_crop(crop_type)
   local overlay = self._crop_overlays[crop_type]
   if not overlay then
      local crop_data = radiant.resources.load_json(crop_type)
      self._crop_overlays[crop_type] = crop_data.components['stonehearth:crop'].plant_overlay_effect
      overlay = self._crop_overlays[crop_type]
   end
   return overlay
end

function FarmingService:harvest_crops(faction, soil_plots)
   if not soil_plots[1] then
      return false
   end
   local town = stonehearth.town:get_town(faction)
   for i, plot in ipairs(soil_plots) do 
      local plot_component = plot:get_component('stonehearth:dirt_plot')
      local plant = plot_component:get_contents()
      if plant then
         town:harvest_resource_node(plant)
      end
   end
   return true
end

return FarmingService