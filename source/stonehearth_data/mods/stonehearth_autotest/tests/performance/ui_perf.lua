local ui_perf = {}

local Point3f = _radiant.csg.Point3f

function ui_perf.hard(autotest)
   autotest:sleep(10 * 1000)
   autotest.ui:set_camera_path_type('ORBIT', { 
      orbit_point = Point3f(0, 0, 0), 
      orbit_distance = 500,
      orbit_period = 10,
      orbit_height = 300 }
      )

   autotest.ui:stress_ui_performance_hard()
   autotest:sleep(9 * 1000)
   autotest:success()
end

function ui_perf.easy(autotest)
   autotest:sleep(10 * 1000)
   autotest.ui:set_camera_path_type('ORBIT', { 
      orbit_point = Point3f(0, 0, 0), 
      orbit_distance = 500,
      orbit_period = 10,
      orbit_height = 300 }
      )

   autotest.ui:stress_ui_performance_easy()
   autotest:sleep(9 * 1000)
   autotest:success()
end


return ui_perf
