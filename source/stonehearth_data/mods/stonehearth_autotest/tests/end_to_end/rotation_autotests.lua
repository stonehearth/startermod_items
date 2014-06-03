local Point3 = _radiant.csg.Point3

local rotation_tests = {}

-- make sure a sensor doesn't detect things out of range.
function rotation_tests.rotate_2x2(autotest)
   local x, y = -8, -8
   for i = 0, 3 do
      local entity = autotest.env:create_entity(x, y, '/stonehearth_autotest/entities/2x2/2x2_0_0.json')
      entity:get_component('mob'):turn_to(i * 90)
      x = x + 4
      break;
   end
   local entity = autotest.env:create_entity(4, 4, 'stonehearth:small_oak_tree')
   autotest.env:create_entity(4, -4, 'stonehearth:female_2')
   autotest:sleep(1000000)
   autotest:success()
end

return rotation_tests
