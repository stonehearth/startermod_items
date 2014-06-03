local Point3 = _radiant.csg.Point3

local rotation_tests = {}

-- make sure a sensor doesn't detect things out of range.
function rotation_tests.rotate_2x2(autotest)
   local x, y = -8, -8
   for i = 0, 3 do
      local _2x2 = autotest.env:create_entity(x, y, '/stonehearth_autotest/entities/2x2/2x2_0_0.json')
      local _3x3 = autotest.env:create_entity(x, y + 6, '/stonehearth_autotest/entities/3x3/3x3_0_0.json')
      _2x2:get_component('mob'):turn_to(i * 90)
      _3x3:get_component('mob'):turn_to(i * 90)
      x = x + 6
   end
   local entity = autotest.env:create_entity(4, 8, 'stonehearth:small_oak_tree')
   autotest.env:create_entity(4, -4, 'stonehearth:female_2')
   autotest:sleep(1000000)
   autotest:success()
end

return rotation_tests
