local constants = require 'constants'
local Array2D = require 'services.server.world_generation.array_2D'
local BlueprintGenerator = require 'services.server.world_generation.blueprint_generator'

local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3
local Point3 = _radiant.csg.Point3
local Rect2 = _radiant.csg.Rect2
local Region2 = _radiant.csg.Region2

local log = radiant.log.create_logger('world_generation')
local GENERATION_RADIUS = 2
local NUM_STARTING_CITIZENS = 7

GameCreationService = class()

function GameCreationService:initialize()
   self._sv = self.__saved_variables:get_data()
   self._generated_citizens = {}
end

function GameCreationService:sign_in_command(session, response)
   stonehearth.player:add_player(session.player_id, 'stonehearth:kingdoms:ascendancy')   
   return {
      version = _radiant.sim.get_version()
   }
end

function GameCreationService:generate_citizens_command(session, response)
   local pop = stonehearth.population:get_population(session.player_id)

   --First destroy all existing citizens.
   for i=1, NUM_STARTING_CITIZENS do
      if self._generated_citizens[i] then
         radiant.entities.destroy_entity(self._generated_citizens[i])      
      end
   end

   --Create a new set of citizens.
   for i=1, NUM_STARTING_CITIZENS do
      self._generated_citizens[i] = self:_generate_citizen(pop)
   end
   response:resolve({})
end

function GameCreationService:new_game_command(session, response, num_tiles_x, num_tiles_y, seed, options)
   local wgs = stonehearth.world_generation
   local blueprint
   local tile_margin

   self:_set_game_options(options)

   wgs:create_new_game(seed, true)

   local generation_method = radiant.util.get_config('world_generation.method', 'default')

   -- Temporary merge code. The javascript client may eventually hold state about the original dimensions.
   if generation_method == 'tiny' then
      tile_margin = 0
      blueprint = wgs.blueprint_generator:get_empty_blueprint(2, 2) -- (2,2) is minimum size
      blueprint:get(1, 1).terrain_type = 'mountains'
      blueprint:get(1, 2).terrain_type = 'foothills'
   else
      -- generate extra tiles along the edge of the map so that we still have a full N x N set of tiles if we embark on the edge
      tile_margin = GENERATION_RADIUS
      num_tiles_x = num_tiles_x + 2*tile_margin
      num_tiles_y = num_tiles_y + 2*tile_margin
      blueprint = wgs.blueprint_generator:generate_blueprint(num_tiles_x, num_tiles_y, seed)
   end

   wgs:set_blueprint(blueprint)

   return self:_get_overview_map(tile_margin)
end

function GameCreationService:_set_game_options(options)
   if not options.enable_enemies then
      stonehearth.game_master:enable_campaign_type('combat', false)
      stonehearth.game_master:enable_campaign_type('ambient_threats', false)
   end
   self.game_options = options
   
   if not self.game_options.starting_talismans then
      self.game_options.starting_talismans = {"stonehearth:carpenter:talisman", "stonehearth:trapper:talisman"}
   end
   
   if not self.game_options.starting_gold then
       self.game_options.starting_gold = 0
   end
end

function GameCreationService:_get_overview_map(tile_margin)
   local wgs = stonehearth.world_generation
   local terrain_info = wgs:get_terrain_info()
   local width, height = wgs.overview_map:get_dimensions()
   local map = wgs.overview_map:get_map()

   -- create an inset map so that embarking on the edge of the map will still get a full N x N set of terrain tiles
   local macro_blocks_per_tile = terrain_info.tile_size / terrain_info.macro_block_size
   local macro_block_margin = tile_margin*macro_blocks_per_tile
   local inset_width = width - 2*macro_block_margin
   local inset_height = height - 2*macro_block_margin
   local inset_map = Array2D(inset_width, inset_height)

   Array2D.copy_block(inset_map, map, 1, 1, 1+macro_block_margin, 1+macro_block_margin, inset_width, inset_height)

   local js_map = inset_map:clone_to_nested_arrays()

   local result = {
      map = js_map,
      map_info = {
         width = inset_width,
         height = inset_height,
         macro_block_margin = macro_block_margin
      }
   }
   return result
end

-- feature_cell_x and feature_cell_y are base 0
function GameCreationService:generate_start_location_command(session, response, feature_cell_x, feature_cell_y, map_info)
   local wgs = stonehearth.world_generation

   -- +1 to convert to base 1
   -- +macro_block_margin to convert from inset coordinates to full coordinates
   feature_cell_x = feature_cell_x + 1 + map_info.macro_block_margin
   feature_cell_y = feature_cell_y + 1 + map_info.macro_block_margin

   local x, z = wgs.overview_map:get_coords_of_cell_center(feature_cell_x, feature_cell_y)

   -- better place to store this?
   wgs.generation_location = Point3(x, 0, z)

   local radius = GENERATION_RADIUS
   local blueprint = wgs:get_blueprint()
   local i, j = wgs:get_tile_index(x, z)

   local generation_method = radiant.util.get_config('world_generation.method', 'default')
   if generation_method == 'small' then
      radius = 1
   end

   -- move (i, j) if it is too close to the edge
   if blueprint.width > 2*radius+1 then
      i = radiant.math.bound(i, 1+radius, blueprint.width-radius)
   end
   if blueprint.height > 2*radius+1 then
      j = radiant.math.bound(j, 1+radius, blueprint.height-radius)
   end
  
   wgs:generate_tiles(i, j, radius)

   response:resolve({})
end

-- returns coordinates of embark location
function GameCreationService:embark_command(session, response)
   local scenario_service = stonehearth.scenario
   local wgs = stonehearth.world_generation

   local x = wgs.generation_location.x
   local z = wgs.generation_location.z
   local y = radiant.terrain.get_point_on_terrain(Point3(x, 0, z)).y

   return {
      x = x,
      y = y,
      z = z
   }
end

function GameCreationService:create_camp_command(session, response, pt)
   -- start the game master service
   stonehearth.calendar:start()
   stonehearth.game_master:start()
   stonehearth.hydrology:start()

   stonehearth.world_generation:set_starting_location(Point2(pt.x, pt.z))

   local town = stonehearth.town:get_town(session.player_id)
   local pop = stonehearth.population:get_population(session.player_id)
   local random_town_name = town:get_town_name()

   -- place the stanfard in the middle of the camp
   local location = Point3(pt.x, pt.y, pt.z)
   local banner_entity = radiant.entities.create_entity('stonehearth:camp_standard', { owner = session.player_id })
   radiant.terrain.place_entity(banner_entity, location, { force_iconic = false })
   town:set_banner(banner_entity)
   radiant.entities.turn_to(banner_entity, constants.placement.DEFAULT_ROTATION)

   -- build the camp
   local camp_x = pt.x
   local camp_z = pt.z
   
   self:_place_citizen_embark(1, pop, camp_x-3, camp_z-3)
   self:_place_citizen_embark(2, pop, camp_x+0, camp_z-3)
   self:_place_citizen_embark(3, pop, camp_x+3, camp_z-3)
   self:_place_citizen_embark(4, pop, camp_x-3, camp_z+3)
   self:_place_citizen_embark(5, pop, camp_x+3, camp_z+3)
   self:_place_citizen_embark(6, pop, camp_x-3, camp_z+0)
   self:_place_citizen_embark(7, pop, camp_x+3, camp_z+0)
   
   self:_place_item(pop, 'stonehearth:decoration:firepit', camp_x, camp_z+3, { force_iconic = false })

   radiant.entities.pickup_item(self._generated_citizens[1], pop:create_entity('stonehearth:resources:wood:oak_log'))
   radiant.entities.pickup_item(self._generated_citizens[2], pop:create_entity('stonehearth:resources:wood:oak_log'))

   -- Spawn initial talismans
   for i, talisman_uri in ipairs (self.game_options.starting_talismans) do
      if i + 2 > NUM_STARTING_CITIZENS then
         break
      end
      radiant.entities.pickup_item(self._generated_citizens[i + 2], pop:create_entity(talisman_uri))
   end

   -- kickstarter pets
   if self.game_options.starting_pets then
       for i, pet_uri in ipairs (self.game_options.starting_pets) do
          local x_offset = -6 + i * 3;
          self:_place_pet(pop, pet_uri, camp_x-x_offset, camp_z-6)
       end
   end

   -- Add starting gold
   local starting_gold = self.game_options.starting_gold;
   if (starting_gold > 0) then
      local inventory = stonehearth.inventory:get_inventory(session.player_id)
      inventory:add_gold(starting_gold)
   end

   return {random_town_name = random_town_name}
end

function GameCreationService:_place_citizen_embark(index, pop, x, z)
   local citizen = self._generated_citizens[index];
   if not citizen then
      citizen = self:_generate_citizen(pop)
      self._generated_citizens[index] = citizen
   end

   radiant.terrain.place_entity(citizen, Point3(x, 1, z))
   radiant.events.trigger_async(stonehearth.personality, 'stonehearth:journal_event', 
                          {entity = citizen, description = 'person_embarks'})
   radiant.entities.turn_to(citizen, constants.placement.DEFAULT_ROTATION)
end

function GameCreationService:_place_pet(pop, uri, x, z)
   local pet = self:_place_item(pop, uri, x, z)
   
   local equipment = pet:add_component('stonehearth:equipment')
   local pet_collar = radiant.entities.create_entity('stonehearth:pet_collar')
   equipment:equip_item(pet_collar)

   local town = stonehearth.town:get_town(pop:get_player_id())   
   town:add_pet(pet)
end

function GameCreationService:_generate_citizen(pop, job, talisman)
   local citizen = pop:create_new_citizen()
   if not job then
      job = 'stonehearth:jobs:worker'
   end
   citizen:add_component('stonehearth:job')
               :promote_to(job, {
                  talisman = talisman
               })
   return citizen
end

function GameCreationService:_place_item(pop, uri, x, z, options)
   local player_id = pop:get_player_id()
   local entity = radiant.entities.create_entity(uri, { owner = player_id })
   radiant.terrain.place_entity(entity, Point3(x, 1, z), options)

   return entity
end

return GameCreationService

