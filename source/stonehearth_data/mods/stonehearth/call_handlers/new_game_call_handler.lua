local MathFns = require 'services.world_generation.math.math_fns'
local BlueprintGenerator = require 'services.world_generation.blueprint_generator'
local game_master = require 'services.game_master.game_master_service'
local personality_service = stonehearth.personality

local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3
local Point3f = _radiant.csg.Point3f
local Rect2 = _radiant.csg.Rect2
local Region2 = _radiant.csg.Region2

local log = radiant.log.create_logger('world_generation')

local NewGameCallHandler = class()

function NewGameCallHandler:new_game(session, response, num_tiles_x, num_tiles_y, seed)
   local wgs = stonehearth.world_generation
   local blueprint

   wgs:initialize(seed, true)

   local generation_method = radiant.util.get_config('world_generation.method', 'default')

   -- Temporary merge code. The javascript client may eventually hold state about the original dimensions.
   if generation_method == 'tiny' then
      blueprint = wgs.blueprint_generator:get_empty_blueprint(2, 2) -- (2,2) is minimum size
      --blueprint = wgs.blueprint_generator:get_static_blueprint()
   else
      blueprint = wgs.blueprint_generator:generate_blueprint(num_tiles_x, num_tiles_y, seed)
   end

   wgs:set_blueprint(blueprint)

   return NewGameCallHandler:get_overview_map(session, response)
end

function NewGameCallHandler:get_overview_map(session, response)
   local wgs = stonehearth.world_generation
   local width, height = wgs.overview_map:get_dimensions()
   local map = wgs.overview_map:get_map():clone_to_nested_arrays()

   local result = {
      width = width,
      height = height,
      map = map
   }
   return result
end

-- feature_cell_x and feature_cell_y are base 0
function NewGameCallHandler:generate_start_location(session, response, feature_cell_x, feature_cell_y)
   -- +1 to convert to base 1
   feature_cell_x = feature_cell_x + 1
   feature_cell_y = feature_cell_y + 1

   local wgs = stonehearth.world_generation
   local x, z = wgs.overview_map:get_coords_of_cell_center(feature_cell_x, feature_cell_y)

   -- TODO: store this better
   wgs.start_x = x
   wgs.start_z = z

   local radius = 2
   local blueprint = wgs:get_blueprint()
   local i, j = wgs:get_tile_index(x, z)

   local generation_method = radiant.util.get_config('world_generation.method', 'default')
   if generation_method == 'small' then
      radius = 1
   end

   -- move (i, j) if it is too close to the edge
   if blueprint.width > 2*radius+1 then
      i = MathFns.bound(i, 1+radius, blueprint.width-radius)
   end
   if blueprint.height > 2*radius+1 then
      j = MathFns.bound(j, 1+radius, blueprint.height-radius)
   end
  
   wgs:generate_tiles(i, j, radius)

   response:resolve({})
end

-- returns coordinates of embark location
function NewGameCallHandler:embark_server(session, response)
   local scenario_service = stonehearth.scenario
   local wgs = stonehearth.world_generation
   local x = wgs.start_x
   local z = wgs.start_z
   local y = radiant.terrain.get_height(Point2(x, z))

   -- reveal scenaraios around the staring location
   scenario_service:reveal_starting_location(x, z)

   return { x = x, y = y, z = z }
end

function NewGameCallHandler:embark_client(session, response)
   _radiant.call('stonehearth:embark_server'):done(
      function (o)
         -- reconcile these with the camera constants later
         local camera_height = 30
         local target_distance = 70
         local camera_service = stonehearth.camera

         local target = Point3f(o.x, o.y, o.z)
         local camera_location = Point3f(o.x, o.y + camera_height, o.z + target_distance)

         -- TODO: fix this hack to get around camera interpolation when camera_service is rewritten
         camera_service._next_position = camera_location
         camera_service:set_position(camera_location)
         camera_service:look_at(target)

         _radiant.call('stonehearth:get_visibility_regions'):done(
            function (o)
               log:info('Visible region uri: %s', o.visible_region_uri)
               log:info('Explored region uri: %s', o.explored_region_uri)
               _radiant.renderer.visibility.set_visibility_regions(o.visible_region_uri, o.explored_region_uri)

               response:resolve({})
            end
         )
      end
   )
end

function NewGameCallHandler:choose_camp_location(session, response)
   self._cursor_entity = radiant.entities.create_entity('stonehearth:camp_standard')
   local re = _radiant.client.create_render_entity(1, self._cursor_entity)
   -- xxx todo, manipulate re to change the way the cursor gets
   -- rendered (e.g. transparent)...

   -- capture the mouse.  Call our _on_mouse_event each time, passing in
   -- the entity that we're supposed to create whenever the user clicks.
   self._capture = _radiant.client.capture_input()
   self._capture:on_input(function(e)
         if e.type == _radiant.client.Input.MOUSE then
            self:_on_mouse_event(e.mouse, response)
            return true
         elseif e.type == _radiant.client.Input.KEYBOARD then
            self:_on_keyboard_event(e.keyboard, response)
            return true
         end
         return false
      end)
end

-- called each time the mouse moves on the client.
function NewGameCallHandler:_on_mouse_event(e, response)
   assert(self._capture, "got mouse event after releasing capture")

   -- query the scene to figure out what's under the mouse cursor
   local s = _radiant.client.query_scene(e.x, e.y)

   -- s.location contains the address of the terrain block that the mouse
   -- is currently pointing to.  if there isn't one, move the workshop
   -- way off the screen so it won't get rendered.
   local pt = s.location and s.location or Point3(0, -100000, 0)

   pt.y = pt.y + 1
   self._cursor_entity:add_component('mob'):set_location_grid_aligned(pt)

   -- if the mouse button just transitioned to up and we're actually pointing
   -- to a box on the terrain, send a message to the server to create the
   -- entity.  this is done by posting to the correct route.
   if e:up(1) and s.location then
      -- destroy our capture object to release the mouse back to the client.  don't
      -- destroy the authoring object yet!  doing so now will result in a brief period
      -- of time where the server side object has not yet been created, yet the client
      -- authoring object has been destroyed.  that leads to flicker, which is ugly.
      self._capture:destroy()
      self._capture = nil

      -- pass "" for the function name so the deafult (handle_request) is
      -- called.  this will return a Deferred object which we can use to track
      -- the call's progress
      _radiant.call('stonehearth:create_camp', pt)
               :always(function ()
                     -- whether the request succeeds or fails, go ahead and destroy
                     -- the authoring entity.  do it after the request returns to avoid
                     -- the ugly flickering that would occur had we destroyed it when
                     -- we uninstalled the mouse cursor
                     _radiant.client.destroy_authoring_entity(self._cursor_entity:get_id())
                     response:resolve({ result = true })
                  end)

   end

   -- return true to prevent the mouse event from propogating to the UI
   return true
end

function NewGameCallHandler:_on_keyboard_event(e, response)
   if e.key == _radiant.client.KeyboardInput.KEY_ESC and e.down then
      self:_destroy_capture()
       _radiant.client.destroy_authoring_entity(self._cursor_entity:get_id())
       response:resolve({ result = false })
   end
   return true
end

--- Destroy our capture object to release the mouse back to the client.
function NewGameCallHandler:_destroy_capture()
   self._capture:destroy()
   self._capture = nil
end

function NewGameCallHandler:create_camp(session, response, pt)
   local faction = stonehearth.population:get_faction('civ', 'stonehearth:factions:ascendancy')
   local town = stonehearth.town:get_town(session.faction)

   -- place the stanfard in the middle of the camp
   local location = Point3(pt.x, pt.y, pt.z)
   local banner_entity = radiant.entities.create_entity('stonehearth:camp_standard')
   radiant.terrain.place_entity(banner_entity, location)
   town:set_banner(banner_entity)

   -- build the camp
   local camp_x = pt.x
   local camp_z = pt.z

   local worker1 = self:place_citizen(camp_x-3, camp_z-3)
   radiant.events.trigger(personality_service, 'stonehearth:journal_event', 
                          {entity = worker1, description = 'person_embarks'})

   local worker2 = self:place_citizen(camp_x+0, camp_z-3)
    radiant.events.trigger(personality_service, 'stonehearth:journal_event', 
                           {entity = worker2, description = 'person_embarks'})

   local worker3 = self:place_citizen(camp_x+3, camp_z-3)
   radiant.events.trigger(personality_service, 'stonehearth:journal_event', 
                          {entity = worker3, description = 'person_embarks'})

   local worker4 = self:place_citizen(camp_x-3, camp_z+3)
   radiant.events.trigger(personality_service, 'stonehearth:journal_event', 
                          {entity = worker4, description = 'person_embarks'})

   local worker5 = self:place_citizen(camp_x+3, camp_z+3)
   radiant.events.trigger(personality_service, 'stonehearth:journal_event', 
                          {entity = worker5, description = 'person_embarks'})

   local worker6 = self:place_citizen(camp_x-3, camp_z+0, 'trapper')
   radiant.events.trigger(personality_service, 'stonehearth:journal_event', 
                          {entity = worker6, description = 'person_embarks'})

   self:place_item('stonehearth:firepit', camp_x, camp_z+3, 'civ')

   radiant.entities.pickup_item(worker1, faction:create_entity('stonehearth:oak_log'))
   radiant.entities.pickup_item(worker2, faction:create_entity('stonehearth:oak_log'))
   radiant.entities.pickup_item(worker3, faction:create_entity('stonehearth:oak_log'))
   radiant.entities.pickup_item(worker4, faction:create_entity('stonehearth:carpenter:saw'))

   -- start the game master service
   --game_master.start()

   return {}
end

function NewGameCallHandler:place_citizen(x, z, profession)
   --TODO: faction denotes which player is playing. Have user pick?
   local faction = stonehearth.population:get_faction('civ','stonehearth:factions:ascendancy')
   local citizen = faction:create_new_citizen()
   if not profession then
      profession = 'worker'
   end
   faction:promote_citizen(citizen, profession)

   radiant.terrain.place_entity(citizen, Point3(x, 1, z))
   return citizen
end

function NewGameCallHandler:place_item(uri, x, z, faction)
   local entity = radiant.entities.create_entity(uri)
   radiant.terrain.place_entity(entity, Point3(x, 1, z))
   if faction then
      entity:add_component('unit_info'):set_faction(faction)
   end
   return entity
end

function NewGameCallHandler:place_stockpile(faction, x, z, w, h)
   w = w and w or 3
   h = h and h or 3

   local location = Point3(x, 1, z)
   local size = { w, h }

   local inventory = stonehearth.inventory:get_inventory(faction)
   inventory:create_stockpile(location, size)
end


return NewGameCallHandler
