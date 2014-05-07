local luabind_tests = {}

-- tests to make sure operator== works on userdata types
function luabind_tests.equality(autotest)
   local e0 = autotest.env:create_entity(0, 0, 'stonehearth:oak_log')
   local f = autotest.env:create_entity(0, 0, 'stonehearth:oak_log')
   local e1 = radiant.get_object(e0:get_id())
   local e2 = e1

   local function test_equality(a, b)
      if not (a == b) then
         autotest:fail('equality of entities failed')
      end
      if a ~= b then
         autotest:fail('equality of entities failed')
      end
   end

   local function test_inequality(a, b)
      if a == b then
         autotest:fail('inequality of entities failed')
      end
      if not (a ~= b) then
         autotest:fail('inequality of entities failed')
      end
   end

   test_inequality(e0, f)
   test_equality(e0, e1)
   test_equality(e0, e2)
   radiant.entities.destroy_entity(e0)

   test_inequality(e0, f)
   test_equality(e0, e1)
   test_equality(e0, e2)
   
   radiant.entities.destroy_entity(f)
   test_equality(e0, f)

   autotest:success()
end

return luabind_tests
