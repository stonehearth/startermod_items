local Point3 = _radiant.csg.Point3

local rotation_tests = {}

-- make sure a sensor doesn't detect things out of range.
function rotation_tests.rotate_2x2(autotest)
   local x, y, h = -8, -8, 3
   local function check_2x2()
      for dx=0,2 do
         for dy=0,2 do
            local pt = Point3(x+dx, h, y+dy)
            autotest:log('testing point %s (dx:%d dy:%d)', pt, dx, dy)
            --autotest.util:fail_if(not _physics:is_standable(entity, pt))
         end
      end
   end

   for i = 0, 3 do
      local _2x2 = autotest.env:create_entity(x, y, '/stonehearth_autotest/entities/2x2/2x2_0_0.json')
      check_2x2()

      local _3x3 = autotest.env:create_entity(x, y + 6, '/stonehearth_autotest/entities/3x3/3x3_0_0.json')
      _2x2:get_component('mob'):turn_to(i * 90)
      _3x3:get_component('mob'):turn_to(i * 90)
      x = x + 6
   end
   autotest:sleep(100)
   autotest:success()
end

function rotation_tests.single_rotation(autotest)
   local person = autotest.env:create_person(8, 8, { profession = 'farmer' })
   local e = autotest.env:create_entity(0, 0, '/stonehearth_autotest/entities/dst_4_0_0/dst_4_0_0.json')

   local rotation
   local function check_destination(rotation)
      local expected_dst = {  -- these are in world coordinates...
         [0]   = Point3( 4,  1,  0),
         [90]  = Point3( 0,  1, -4),
         [180] = Point3(-4,  1,  0),
         [270] = Point3( 0,  1,  4),
      }
      local local_dst
      for cube in e:get_component('destination'):get_region():get():each_cube() do
         local_dst = cube.min
         break
      end
      local rotated_dst = radiant.entities.local_to_world(local_dst, e)
      autotest:log("dst region: checking %s transformed to %s against %s (%d degree rotation)", local_dst, rotated_dst, expected_dst[rotation], rotation)
      autotest.util:fail_if(rotated_dst ~= expected_dst[rotation])
   end
   local function check_pathfinder(rotation, method)
      -- expected adjacent points where the path finder should finish, in world coordinates.
      local expected_finish = {
         [0]   = Point3( 5,  1,  0),
         [90]  = Point3( 0,  1, -6),
         [180] = Point3(-6,  1, -1),
         [270] = Point3(-1,  1,  5),
      }
      local local_adjacent
      for cube in e:get_component('destination'):get_adjacent():get():each_cube() do
         local_adjacent = cube.min
         break
      end

      local path
      if method == 'direct' then         
         path = _radiant.sim.create_direct_path_finder(person)
                                                :set_destination_entity(e)
                                                :get_path()
      elseif method == 'astar' then
         local pf
         pf = _radiant.sim.create_astar_path_finder(person, 'find path to entity')
                        :add_destination(e)
                        :set_solved_cb(function (p)                             
                              autotest:log('solved')
                              path = p
                              autotest:resume()
                           end)
                        :start()
         autotest:log('suspending')
         autotest:suspend()
         pf:stop()
      else
         autotest:fail('unknown pathfinder method %s', method)
      end

      autotest.util:fail_if(not path, '"%s pathfinder: could not find path to entity', method)
      local finish_pt = path:get_finish_point()
      autotest:log("%s pathfinder: checking finish %s against expected %s (%d degree rotation)", method, finish_pt, expected_finish[rotation], rotation)
      autotest.util:fail_if(finish_pt ~= expected_finish[rotation])
   end

   for rotation=0, 270, 90 do
      e:get_component('mob'):turn_to(rotation)
      check_destination(rotation)
      check_pathfinder(rotation, 'direct')
      check_pathfinder(rotation, 'astar')
   end

   --autotest.env:create_entity(0, 0, 'stonehearth:small_boulder')
   autotest:sleep(100)
   autotest:success()
end

return rotation_tests
