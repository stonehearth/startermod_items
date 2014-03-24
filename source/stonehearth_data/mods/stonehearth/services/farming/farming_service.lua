FarmingService = class()

function FarmingService:__init()
   self:_load_dirt_descriptions()
   self._crop_details = {}
   self:_load_initial_crops()
end

--- Track the crops currently available to each community
--  Start the town with a certain number of crops
function FarmingService:initialize()
   self._data = {
      -- we probably want different crop inventories per town, but right now there's
      -- at most one town per player (and only 1 player.  ha!).  until we get that
      -- straightend out, let's just assume a player can use all the crops in all
      -- his towns
      player_crops = {}
   }
   self.__saved_variables = radiant.create_datastore(self._data)
   self.__saved_variables:mark_changed()
end

function FarmingService:restore(saved_variables)
end

function FarmingService:create_new_field(session, location, size)
   local entity = radiant.entities.create_entity('stonehearth:farmer:field')   
   radiant.terrain.place_entity(entity, location)
   
   local town = stonehearth.town:get_town(session.player_id)

   local unit_info = entity:get_component('unit_info')
   unit_info:set_display_name('New Field foo')
   unit_info:set_faction(session.faction)
   unit_info:set_player_id(session.player_id)

   local farmer_field = entity:get_component('stonehearth:farmer_field')
   farmer_field:create_dirt_plots(town, location, size)

   return entity
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
   self._initial_crops = radiant.resources.load_json('stonehearth:farmer:initial_crops')

   --Pre-load the details for the non-crop "fallow"
   self._crop_details['fallow'] = self._initial_crops.data.fallow
end

--- Given a new crop type, record some important things about it
function FarmingService:get_crop_details(crop_type)
   local details = self._crop_details[crop_type]
   if not details then
      local crop_data = radiant.resources.load_json(crop_type)
      details = {}
      details.uri = crop_type
      details.overlay = crop_data.components['stonehearth:crop'].plant_overlay_effect
      details.name = crop_data.components.unit_info.name
      details.description = crop_data.components.unit_info.description
      details.icon = crop_data.components.unit_info.icon
      self._crop_details[crop_type] = details
   end
   return details
end

function FarmingService:_get_overlay_for_crop(crop_type)
   return self:get_crop_details(crop_type).overlay
end


function FarmingService:get_dirt_descriptions(dirt_type)
   local dirt_details = self._dirt_data[dirt_type]
   return dirt_details.unit_info_name, dirt_details.unit_info_description
end

--- Get the crop types available to a player. Start with a couple if there are none so far.
function FarmingService:get_all_crop_types(session)
   return self:_get_crop_list(session)
end

--- Add a new crop type to the player
function FarmingService:add_crop_type(session, new_crop_uri)
   local crop_list = self:_get_crop_list(session)
   table.insert(crop_list, new_crop_uri)
   return crop_list
end

function FarmingService:_get_crop_list(session)
   local player_id = session.player_id
   local crop_list = self._data.player_crops[player_id]
   if not crop_list then
      -- start out with the default crops for this player's kingdom.
      crop_list = {}
      local kingdom_crops = self._initial_crops[session.kingdom]
      if kingdom_crops then
         for i, crop in ipairs(kingdom_crops) do
            crop_list[i] = {
               crop_type = crop.crop_type,
               crop_info = self:get_crop_details(crop.crop_type),
               quantity = crop.quantity
            }
         end
      end
      self._data.player_crops[player_id] = crop_list
   end
   return crop_list
end

--- Tell farmers to plan the crop_type in the designated locations
-- @param faction: the group that should be planting the crop
-- @param soil_plots: array of entities on top of which to plant the crop
-- @param crop_type: the name of the thing to plant (ie, stonehearth:corn, etc)
function FarmingService:plant_crop(player_id, soil_plots, crop_type, player_speficied, auto_plant, auto_harvest)
   if not soil_plots[1] or not crop_type then
      return false
   end
   local town = stonehearth.town:get_town(player_id)
   for i, plot in ipairs(soil_plots) do
      --Tell the dirt plot whether the player wanted to autoreplant/autoharvest this plot
      --TODO: right now these are always false for manual plant commands. Base this on the UI.
      --TODO: maybe change this from the UI, instead of from the plant command
      local dirt_plot_component = plot:get_component('stonehearth:dirt_plot')
      dirt_plot_component:set_player_override(player_speficied, auto_plant, auto_harvest)

      --TODO: store these tasks, so they can be cancelled
      local overlay_effect = self:_get_overlay_for_crop(crop_type)
      town:create_farmer_task('stonehearth:plant_crop', {target_plot = plot, 
                                                         dirt_plot_component = dirt_plot_component,
                                                         crop_type = crop_type})
                              :set_source(plot)
                              :add_entity_effect(plot, overlay_effect)
                              :set_name('plant_crop')
                              :set_priority(stonehearth.constants.priorities.farmer_task.PLANT)
                              :once()
                              :start()
   end
   return true
end

function FarmingService:harvest_crops(session, soil_plots)
   if not soil_plots[1] then
      return false
   end
   local town = stonehearth.town:get_town(session.player_id)
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