local Point3 = _radiant.csg.Point3
local TraceCategories = _radiant.dm.TraceCategories

local dm_map_autotests = {}

local function verify_map(autotest, m, expected)
	for k, v in m:each() do
		if expected[k] ~= v then
			autotest:fail()
		end
	end
	for k, v in pairs(expected) do
		if m:get(k) ~= v then
			autotest:fail()
		end
	end
end

function dm_map_autotests.basic_functionality(autotest)
	local m = _radiant.sim.alloc_number_map()
	verify_map(autotest, m, {})

	m:add(1, true)
	verify_map(autotest, m, { true })

	m:add(2, "a string")
	verify_map(autotest, m, { true, "a string" })
	
	m:remove(2)
	verify_map(autotest, m, { true })
	
	autotest:success()
end

function dm_map_autotests.remove_during_iteration(autotest)
	local m = _radiant.sim.alloc_number_map()
	local verify = {}
	for i=1,10 do
	   m:add(i, i)
	   verify[i] = i
	   verify_map(autotest, m, verify)
	end
	for k, v in m:each() do
	   m:remove(k)
	   verify[k] = nil
	   verify_map(autotest, m, verify)   
	end
	autotest.util:fail_if(m:get_size() ~= 0)
	autotest:success()
end

return dm_map_autotests
