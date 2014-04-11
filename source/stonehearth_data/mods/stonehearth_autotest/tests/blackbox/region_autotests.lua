local Point3 = _radiant.csg.Point3
local region_tests = {}

function region_tests.trace_fire_count(autotest)
	local e = radiant.entities.create_entity()
   local dst = e:add_component('destination')

	local count, rgn

	local trace = dst:trace_region('testing tracing')
								:on_changed(function()
										count = count + 1
									end)

	autotest:log('verify that push_object_state() only fires the trace once.')
	count = 0
	trace:push_object_state()
	autotest.util:assert(count == 1, 'trace count %d != 1 after push_object_state', count)

	autotest:log('verify that the trace only fires once when setting a new region')
	count = 0
	rgn = _radiant.sim.alloc_region()
	dst:set_region(rgn)
	autotest.util:assert(count == 1, 'trace count %d != 1 after set_region', count)

	autotest:log('verify that the trace only fires once when modifying the current region')
	count = 0
	rgn:modify(function(r)
			r:add_point(Point3(1, 1, 1))
		end)
	autotest.util:assert(count == 1, 'trace count %d != 1 after modifying region', count)

	radiant.entities.destroy_entity(e)
	trace:destroy()
	autotest:success()
end

return region_tests
