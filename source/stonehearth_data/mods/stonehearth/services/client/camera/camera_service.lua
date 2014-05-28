local Vec3 = _radiant.csg.Point3f
local Quat = _radiant.csg.Quaternion
local Ray = _radiant.csg.Ray3
local log = radiant.log.create_logger('camera')
local PlayerCameraController = require 'services.client.camera.player_camera_controller'

local CameraService = class()

function CameraService:initialize()
   self._sv = self.__saved_variables:get_data()
   
   if not self._sv.controllers then
      self._sv.controllers = {}
      self._sv.camera_disabled = false
      self:push_controller(PlayerCameraController, PlayerCameraController(radiant.create_datastore()))
   else
      for idx, c in pairs(self._sv.controllers) do
         self._sv.controllers[idx] = c.ctor(c.controller)
      end
   end

   self._frame_trace = _radiant.client.trace_render_frame()

   self._frame_trace:on_frame_start('update camera', function(now, alpha, frame_time, frame_time_wallclock)
         self:_update_camera(frame_time_wallclock)
         return true
      end)
end


function CameraService:push_controller(controller_ctor, controller)
   local controller = {
      ctor = controller_ctor,
      controller = controller
   }

   table.insert(self._sv.controllers, controller)

   self.__saved_variables:mark_changed()
end


function CameraService:pop_controller()
   table.remove(self._sv.controllers, #self._sv.controllers)
end


function CameraService:controller_top()
   return self._sv.controllers[#self._sv.controllers].controller
end


function CameraService:_update_camera(frame_time_wallclock)
   if self._sv.camera_disabled then
      return
   end

   local new_pos, new_rot = self:controller_top():update(frame_time_wallclock)

   _radiant.renderer.camera.set_position(new_pos)
   _radiant.renderer.camera.set_orientation(new_rot)
end


function CameraService:get_left()
   return _radiant.renderer.camera.get_left()
end


function CameraService:get_forward()
   --Ugh.  Just...just trust me on this.
   return _radiant.renderer.camera.get_forward():scaled(-1)
end


function CameraService:get_position()
   return _radiant.renderer.camera.get_position()
end


function CameraService:set_position(position)
   _radiant.renderer.camera.set_position(position)

   -- We probably want to propagate the position-change to all controllers
   -- in the stack, but hold off for now to see how well this works.
   self:controller_top():set_position(position)
end


function CameraService:look_at(pos)
   local rot = Quat()
   rot:look_at(self:get_position(), pos)
   rot:normalize()
   _radiant.renderer.camera.set_orientation(rot)

   -- We probably want to propagate the look-at-change to all controllers
   -- in the stack, but hold off for now to see how well this works.
   self:controller_top():look_at(pos)
end


function CameraService:enable_camera_movement(enabled)
   self._sv.camera_disabled = not enabled

   self:controller_top():enable_camera(enabled)
end


return CameraService
