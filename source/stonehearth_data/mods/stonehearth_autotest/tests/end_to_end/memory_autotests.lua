local Point3f = _radiant.csg.Point3f

local memory_tests = {}

function memory_tests.entity_spawn(autotest)
   local timespan = 2400000
   for i = 1, 50000 do
      local es = {}
      for i = 1, 7 do
         for j = 1, 7 do
            local e = autotest.env:create_person(i, j, { })
            table.insert(es, e)
         end
      end
      autotest:sleep(25)

      for _, e in ipairs(es) do
         radiant.entities.destroy_entity(e)
      end
   end


   autotest:sleep(timespan)
end

return memory_tests
