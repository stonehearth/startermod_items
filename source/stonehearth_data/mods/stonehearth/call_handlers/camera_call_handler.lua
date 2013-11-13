local camera = require 'services.camera.camera_service'

local CameraCallHandler = class()

function CameraCallHandler:camera_look_at_entity(session, request, entity)
   camera:look_at_entity(entity)
   request:resolve({})
end

return CameraCallHandler
