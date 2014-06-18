local Point3f = _radiant.csg.Point3f

local mob_of_mobs = {}

function mob_of_mobs.mob(autotest)

   local orbit_point = Point3f(0, 0, 0)

   for x=-10,10,2 do
      for z=-10,10,2 do
         autotest.env:create_person(x + orbit_point.x, orbit_point.z + z)
      end
   end

   autotest.ui:set_camera_path_type('ORBIT', { 
      orbit_point = orbit_point, 
      orbit_distance = 50,
      orbit_period = 10,
      orbit_height = 70 }
      )

   autotest:sleep(30*1000)
   autotest:success()
end

return mob_of_mobs
