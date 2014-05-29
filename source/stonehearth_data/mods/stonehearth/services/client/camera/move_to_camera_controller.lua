local Vec3 = _radiant.csg.Point3f
local Quat = _radiant.csg.Quaternion
local Ray = _radiant.csg.Ray3

local MoveToCameraController = class()

function MoveToCameraController:__init(datastore, end_pos, travel_time)
   self.__saved_variables = datastore
   self._sv = self.__saved_variables:get_data()

   if not self._sv.end_pos then
      self._sv.elapsed_time = 0
      self._sv.travel_time = travel_time
      self._sv.end_pos = end_pos
   end
end



function MoveToCameraController:enable_camera(enabled)
   -- Ignore; we don't care whether or not we get disabled.
end


function MoveToCameraController:set_position(pos)
   -- Ignore; we don't care what higher level controllers do.
end

function MoveToCameraController:look_at(where)
   -- Ignore; we don't care what higher level controllers do.
end


function MoveToCameraController:update(frame_time)
   self._sv.elapsed_time = self._sv.elapsed_time + frame_time
   local lerp_time = self._sv.elapsed_time / self._sv.travel_time
   local lerp_pos = stonehearth.camera:get_position():lerp(self._sv.end_pos, lerp_time)
   local rot = Quat()
   rot:look_at(Vec3(0, 0, 0), stonehearth.camera:get_forward())
   rot:normalize()

   -- If we're at the end of the lerp, pop ourselves!
   if lerp_pos:distance_to(self._sv.end_pos) < 0.1 then
      stonehearth.camera:pop_controller()
      -- Make sure that all other controllers now know where the camera has
      -- moved.
      stonehearth.camera:set_position(self._sv.end_pos)
   end

   return lerp_pos, rot
end

return MoveToCameraController
