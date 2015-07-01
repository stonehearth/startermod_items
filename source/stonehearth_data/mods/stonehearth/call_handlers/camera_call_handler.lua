local camera = stonehearth.camera

local MoveToCameraController = require 'services.client.camera.move_to_camera_controller'
local CameraCallHandler = class()

local camera_tracker = nil
function CameraCallHandler:camera_look_at_entity(session, request, entity)
   if not entity or type(entity) == 'string' or not entity:is_valid() then
      return
   end

   -- We want to maintain the orientation of the camera, and (for now) the height
   -- of the camera.
   local cam_height = camera:get_position().y
   local entity_pos = entity:get_component('mob'):get_location()

   local parent = entity:get_component('mob'):get_parent()
   if parent then
      local parent_mob = parent:get_component('mob')
      if parent_mob then
        entity_pos = entity_pos + parent_mob:get_location()
      end
   end

   local t = (cam_height - entity_pos.y) / -camera:get_forward().y

   if t > 70 then
      t = 70
   end
   local cam_pos = entity_pos + -camera:get_forward():scaled(t)

   camera:push_controller('stonehearth:move_to_camera_controller', cam_pos, 1000)
   request:resolve({})
end

function CameraCallHandler:enable_camera_movement(session, request, enable)
   camera:enable_camera_movement(enable)
end

function CameraCallHandler:get_camera_tracker(session, request)
   if not camera_tracker then
      camera_tracker = _radiant.client.create_datastore()

      radiant.events.listen(camera, 'stonehearth:camera:update', function(e)
            if e then
                camera_tracker:set_data({
                   pan = e.pan,
                   zoom = e.zoom,
                  orbit = e.orbit
               })
            end
         end)
   end

   return { camera_tracker = camera_tracker }
end

return CameraCallHandler
