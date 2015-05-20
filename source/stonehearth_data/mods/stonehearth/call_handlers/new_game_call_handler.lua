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

local NewGameCallHandler = class()

function NewGameCallHandler:embark_client(session, response)
   _radiant.call_obj('stonehearth.game_creation', 'embark_command'):done(
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
         local clip_height = self:_get_starting_clip_height(location)
         stonehearth.subterranean_view:set_clip_height(clip_height)

         _radiant.call_obj('stonehearth.game_creation','create_camp_command', location)
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

function NewGameCallHandler:_get_starting_clip_height(starting_location)
   local step_size = constants.mining.Y_CELL_SIZE
   local quantized_height = math.floor(starting_location.y / step_size) * step_size
   local next_step = quantized_height + step_size
   local clip_height = next_step - 1 -- -1 to remove the ceiling
   return clip_height
end

return NewGameCallHandler
