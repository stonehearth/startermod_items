local Vec3 = _radiant.csg.Point3f
local Quat = _radiant.csg.Quaternion
local Ray = _radiant.csg.Ray3

local log = radiant.log.create_logger('autotest_camera')

local OrbitCameraController = class()

function OrbitCameraController:initialize(orbit_point, orbit_distance, orbit_height, orbit_period)
   self._sv.orbit_point = Vec3(orbit_point.x,orbit_point.y,orbit_point.z)
   self._sv.orbit_distance = orbit_distance
   self._sv.elapsed_time = 0
   self._sv.frame_num = 0
   self._sv.frame_time = 0
   self._sv.orbit_period = orbit_period
   self._sv.orbit_height = orbit_height
end


function OrbitCameraController:enable_camera(enabled)
   -- Ignore; we don't care whether or not we get disabled.
end


function OrbitCameraController:set_position(pos)
   -- Ignore; we don't care what higher level controllers do.
end


function OrbitCameraController:look_at(where)
   -- Ignore; we don't care what higher level controllers do.
end


function OrbitCameraController:update(frame_time)
   self._sv.elapsed_time = self._sv.elapsed_time + frame_time

   local orbit_period = self._sv.orbit_period * 1000 / (3.1415 * 2)

   local theta = self._sv.elapsed_time / orbit_period
   local pos = Vec3(math.sin(theta) * self._sv.orbit_distance, self._sv.orbit_height, math.cos(theta) * self._sv.orbit_distance)

   local rot = Quat()
   rot:look_at(pos, self._sv.orbit_point)
   rot:normalize()

   self._sv.frame_time = self._sv.frame_time + frame_time
   if self._sv.frame_num % 2 == 0 then
      log:write(0, 'Perf:%.2f:%d', 2 * (1000.0 / self._sv.frame_time), _radiant.renderer.perf.get_tri_count())
      self._sv.frame_time = 0
   end
   self._sv.frame_num = self._sv.frame_num + 1
   return pos, rot
end

return OrbitCameraController
