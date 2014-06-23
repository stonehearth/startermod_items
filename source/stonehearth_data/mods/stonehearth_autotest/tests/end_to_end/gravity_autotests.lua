local Point3f = _radiant.csg.Point3f

local gravity_autotests = {}

function gravity_autotests.falling_log(autotest)
   local tree = autotest.env:create_entity(8, 8, 'stonehearth:small_oak_tree')
   local log = autotest.env:create_entity(8, 8, 'stonehearth:oak_log')

   local falling = false
   local trace = log:get_component('mob'):trace_in_free_motion('autotest')
                        :on_changed(function(in_motion)
                              if in_motion then
                                 falling = true
                              else
                                 if falling then
                                    autotest:success()
                                 end
                              end
                           end)                     
   autotest:sleep(10)
   radiant.entities.destroy_entity(tree)
   autotest:sleep(4000000)
   autotest:fail()
end

return gravity_autotests
