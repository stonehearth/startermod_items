local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3

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
   self:_register_score_functions()

   self.__saved_variables = radiant.create_datastore(self._data)
   self.__saved_variables:mark_changed()

end

function FarmingService:restore(saved_variables)
end

function FarmingService:create_new_field(session, location, size)
   -- A little sanitization: what we get from the client isn't exactly a Point3
   location = Point3(location.x, location.y, location.z)
   local entity = radiant.entities.create_entity('stonehearth:farmer:field')   
   radiant.terrain.place_entity(entity, location)

   self:_add_region_components(entity, size)
   
   local town = stonehearth.town:get_town(session.player_id)

   local unit_info = entity:get_component('unit_info')
   unit_info:set_faction(session.faction)
   unit_info:set_player_id(session.player_id)

   local farmer_field = entity:get_component('stonehearth:farmer_field')
   farmer_field:create_dirt_plots(town, location, size)

   return entity
end
   
function FarmingService:_load_dirt_descriptions()
   local dirt_data = radiant.resources.load_json('stonehearth:tilled_dirt')
   self._dirt_data = {}
   self._dirt_data['furrow'] = dirt_data.components.model_variants['furrow']
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

function FarmingService:get_overlay_for_crop(crop_type)
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

--Returns true if the player/kingdom combination has the crop in question, 
--false otherwise
function FarmingService:has_crop_type(session, crop_type_name)
   for i, crop_data in ipairs(self:_get_crop_list(session)) do
      if crop_data.crop_type == crop_type_name then
         return true
      end
   end   
   return false
end

--- Add a new crop type to a specific player
function FarmingService:add_crop_type(session, new_crop_type, quantity)
   local crop_list = self:_get_crop_list(session)
   local crop_data = {
            crop_type = new_crop_type,
            crop_info = self:get_crop_details(new_crop_type),
            quantity = quantity
         }
   table.insert(crop_list, crop_data)
   return crop_list
end

function FarmingService:_add_region_components(entity, size)
   local destination_component = entity:add_component('destination')
   local collision_component = entity:add_component('region_collision_shape')
   local boxed_bounds = _radiant.sim.alloc_region()

   boxed_bounds:modify(
      function (region3)
         region3:add_unique_cube(
            Cube3(
               -- recall that regions in components are is in local coordiantes
               Point3(0, 0, 0),
               Point3(size.x, 1, size.y)
            )
         )
      end
   )

   destination_component:set_region(boxed_bounds)
                        :set_auto_update_adjacent(true)
   collision_component:set_region(boxed_bounds)
                      :set_region_collision_type(_radiant.om.RegionCollisionShape.NONE)
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

function FarmingService:plant_crop(player_id, crop)
   local town = stonehearth.town:get_town(player_id)
   return town:plant_crop(crop)
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
         town:harvest_crop(plant)
      end
   end
   return true
end

function FarmingService:_register_score_functions()
   --If the entity is a farm, register the score
   stonehearth.score:add_aggregate_eval_function('net_worth', 'agriculture', function(entity, agg_score_bag)
      if entity:get_component('stonehearth:farmer_field') then
         agg_score_bag.agriculture = agg_score_bag.agriculture + self:_get_score_for_farm(entity)
      end
   end)
end

function FarmingService:_get_score_for_farm(entity)
   local field_component = entity:get_component('stonehearth:farmer_field')
   local field_size = field_component:get_size()
   local field_contents = field_component:get_contents()
   local aggregate_score = 0

   for x=1, field_size.x do
      for y=1, field_size.y do
         local field_spacer = field_contents[x][y]
         if field_spacer then
            local dirt_plot_component = field_spacer:get_component('stonehearth:dirt_plot')
            if dirt_plot_component then
               --add a score based on the quality of the dirt
               local fertility, moisture = dirt_plot_component:get_fertility_moisture()
               aggregate_score = aggregate_score + fertility/10

               --add a score for the plant if there's a plant in the dirt
               local crop = dirt_plot_component:get_contents()
               if crop then
                  --TODO: change this value based on a score in the crop's json
                  aggregate_score = aggregate_score + 1
               end
            end

         end
      end
   end
   return aggregate_score
end

return FarmingService