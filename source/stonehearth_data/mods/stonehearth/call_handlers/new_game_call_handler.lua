local Array2D = require 'services.server.world_generation.array_2D'
local BlueprintGenerator = require 'services.server.world_generation.blueprint_generator'
local personality_service = stonehearth.personality
local linear_combat_service =  stonehearth.linear_combat

local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3
local Point3 = _radiant.csg.Point3
local Rect2 = _radiant.csg.Rect2
local Region2 = _radiant.csg.Region2

local log = radiant.log.create_logger('world_generation')
local GENERATION_RADIUS = 2

local NewGameCallHandler = class()

function NewGameCallHandler:sign_in(session, response, num_tiles_x, num_tiles_y, seed)
   local town = stonehearth.town:get_town(session.player_id)
   if not town then
      stonehearth.town:add_town(session)
      stonehearth.inventory:add_inventory(session)
      stonehearth.population:add_population(session, 'stonehearth:kingdoms:ascendancy')
   end

   return {
      version = _radiant.sim.get_version()
   }
end 

function NewGameCallHandler:set_game_options(session, response, options)
   linear_combat_service:enable(options.enable_enemies)
   return true
end

function NewGameCallHandler:new_game(session, response, num_tiles_x, num_tiles_y, seed)
   local wgs = stonehearth.world_generation
   local blueprint
   local tile_margin

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

   return NewGameCallHandler:_get_overview_map(tile_margin)
end

function NewGameCallHandler:_get_overview_map(tile_margin)
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
function NewGameCallHandler:generate_start_location(session, response, feature_cell_x, feature_cell_y, map_info)
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
function NewGameCallHandler:embark_server(session, response)
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

function NewGameCallHandler:embark_client(session, response)
   _radiant.call('stonehearth:embark_server'):done(
      function (o)
         local camera_height = 30
         local target_distance = 70
         local camera_service = stonehearth.camera

         local target = Point3(o.x, o.y, o.z)
         local camera_location = Point3(target.x, target.y + camera_height, target.z + target_distance)

         camera_service:set_position(camera_location)
         camera_service:look_at(target)

         _radiant.call('stonehearth:get_visibility_regions'):done(
            function (o)
               log:info('Visible region uri: %s', o.visible_region_uri)
               log:info('Explored region uri: %s', o.explored_region_uri)
               stonehearth.renderer:set_visibility_regions(o.visible_region_uri, o.explored_region_uri)
               response:resolve({})
            end
         )
      end
   )
end

function NewGameCallHandler:choose_camp_location(session, response)
   stonehearth.selection:select_location()
      :use_ghost_entity_cursor('stonehearth:camp_standard_ghost')
      :done(function(selector, location, rotation)
         _radiant.call('stonehearth:create_camp', location)
            :done( function(o)
                  response:resolve({result = true, townName = o.random_town_name })
               end)
            :fail(function(result)
                  response:reject(result)
               end)
            :always(function ()
                  selector:destroy()
               end)
         end)
      :fail(function(selector)
            selector:destroy()
            response:reject('no location')
         end)
      :go()
end

function NewGameCallHandler:create_camp(session, response, pt)
   stonehearth.world_generation:set_starting_location(Point2(pt.x, pt.z))

   local town = stonehearth.town:get_town(session.player_id)
   local pop = stonehearth.population:get_population(session.player_id)
   local random_town_name = town:get_town_name()

   -- place the stanfard in the middle of the camp
   local location = Point3(pt.x, pt.y, pt.z)
   local banner_entity = radiant.entities.create_entity('stonehearth:camp_standard')
   radiant.terrain.place_entity(banner_entity, location, { force_iconic = false })
   town:set_banner(banner_entity)
   radiant.entities.turn_to(banner_entity, 180)

   -- build the camp
   local camp_x = pt.x
   local camp_z = pt.z

   local function place_citizen_embark(x, z, job, talisman)
      local citizen = self:place_citizen(pop, x, z, job, talisman)
      radiant.events.trigger_async(personality_service, 'stonehearth:journal_event', 
                             {entity = citizen, description = 'person_embarks'})

      radiant.entities.turn_to(citizen, 180)
      return citizen
   end

   local worker1 = place_citizen_embark(camp_x-3, camp_z-3)
   local worker2 = place_citizen_embark(camp_x+0, camp_z-3)
   local worker3 = place_citizen_embark(camp_x+3, camp_z-3)
   local worker4 = place_citizen_embark(camp_x-3, camp_z+3)
   local worker5 = place_citizen_embark(camp_x+3, camp_z+3)
   local worker6 = place_citizen_embark(camp_x-3, camp_z+0)
   local worker7 = place_citizen_embark(camp_x+3, camp_z+0)   
   
   self:place_item(pop, 'stonehearth:firepit', camp_x, camp_z+3, { force_iconic = false })

   radiant.entities.pickup_item(worker1, pop:create_entity('stonehearth:oak_log'))
   radiant.entities.pickup_item(worker2, pop:create_entity('stonehearth:oak_log'))
   radiant.entities.pickup_item(worker3, pop:create_entity('stonehearth:trapper:knife_talisman'))
   radiant.entities.pickup_item(worker4, pop:create_entity('stonehearth:carpenter:saw_talisman'))

   -- start the game master service
   --stonehearth.game_master.start()

   return {random_town_name = random_town_name}
end

function NewGameCallHandler:place_citizen(pop, x, z, job, talisman)
   local citizen = pop:create_new_citizen()
   if not job then
      job = 'stonehearth:jobs:worker'
   end
   pop:promote_citizen(citizen, job, talisman)

   radiant.terrain.place_entity(citizen, Point3(x, 1, z))
   return citizen
end

function NewGameCallHandler:place_item(pop, uri, x, z, options)
   local entity = radiant.entities.create_entity(uri)
   radiant.terrain.place_entity(entity, Point3(x, 1, z), options)

   entity:add_component('unit_info')
            :set_player_id(pop:get_player_id())

   return entity
end

-- xxx, change this whole town name business to a unit_info component on the town entity (created in town.lua)
function NewGameCallHandler:get_town_name(session, response)
   local town = stonehearth.town:get_town(session.player_id)
   if town then
      return {townName = town:get_town_name()}
   else
      return {townName = 'Defaultville'}
   end
end

function NewGameCallHandler:get_town_entity(session, response)
   local town = stonehearth.town:get_town(session.player_id)
   local entity = town:get_entity()
   return { town = entity }
end

function NewGameCallHandler:set_town_name(session, response, town_name)
   local town = stonehearth.town:get_town(session.player_id)
   town:set_town_name(town_name)
   return true
end

return NewGameCallHandler
