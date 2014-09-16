local Point3 = _radiant.csg.Point3

local perf_camera_orbit = {}

function perf_camera_orbit.orbit(autotest)
   autotest.ui:set_camera_path_type('ORBIT', { 
      orbit_point = Point3(0, 0, 0), 
      orbit_distance = 500,
      orbit_period = 10,
      orbit_height = 300 }
      )

   autotest:sleep(30*1000)
   autotest:success()
end

return perf_camera_orbit
