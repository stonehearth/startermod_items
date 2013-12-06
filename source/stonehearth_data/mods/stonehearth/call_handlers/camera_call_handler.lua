local camera = require 'services.camera.camera_service'

local CameraCallHandler = class()

local camera_tracker = nil
local trace
function CameraCallHandler:camera_look_at_entity(session, request, entity)
   camera:look_at_entity(entity)
   request:resolve({})
end

function CameraCallHandler:get_camera_tracker(session, request)
   if not camera_tracker then
      camera_tracker = _radiant.client.create_data_store()
      trace = camera_tracker:trace_data('checking')
                      :on_changed(function()
                        radiant.log.info('boo!')
                      end)
      
      radiant.events.listen(camera, 'stonehearth:camera:update', self, self.update_camera_tracker)
      self:update_camera_tracker()
   end

   return { camera_tracker = camera_tracker }
end

function CameraCallHandler:update_camera_tracker(e)
   if e then
       camera_tracker:update({
          pan = e.pan,
          zoom = e.zoom,
          orbit = e.orbit
       })
   end
end

return CameraCallHandler
