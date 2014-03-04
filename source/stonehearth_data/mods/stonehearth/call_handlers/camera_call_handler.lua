local camera = stonehearth.camera

local CameraCallHandler = class()

local camera_tracker = nil
function CameraCallHandler:camera_look_at_entity(session, request, entity)
   camera:look_at_entity(entity)
   request:resolve({})
end

function CameraCallHandler:enable_camera_movement(session, requiest, enable)
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
