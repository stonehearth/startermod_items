local Entity = _radiant.om.Entity

local CounterEncounter = class()

--[[
This encounter counts the number of times it runs and saves this information in
the user-specified ctx.ctx_registration_variable.

If it manages to run X times, it produces its success out edge.
Otherwise, it produces it's failure out edge.
Both are speciifed in the out_edges variable in the counter_info. 

Optionally, you can pass it a 'reference_entity'. The "instance" only "counts" if the
reference entity still exists and is the same one as last time.
If it doesn't exist, then the encounter does nothing. If it exists, but is a different
entity, than the one stored in context from last time, the counter starts again.

Sample JSON

"counter_info" : {
	"out_edges" : {
        "fail" : "wait_for_ogo_call",
        "success" : "wait_for_ogo_call"
    },
	"times" : 3,
	"ctx_registration_variable" : "raidcounter", 
	"optional_reference_entity" : "goblin_raiding_camp_1.boss",
}

]]

function CounterEncounter:initialize()
end

function CounterEncounter:restore()
end

function CounterEncounter:activate()
	self._log = radiant.log.create_logger('game_master.encounters.counter')
end

-- return the edge to transition to, which is simply the result of the encounter.
-- this will be one of the edges listed in the `out_edges` (refuse, fail, or success)
--
function CounterEncounter:get_out_edge()
   return self._sv.resolved_out_edge
end

function CounterEncounter:start(ctx, info)
	assert(info.times)
	assert(info.ctx_registration_variable)

	if not self:_should_progress_encounter(ctx, info) then
		return 
	end

	--If we got here, it's because we don't need a ref entity, or we do and they exist

	if not ctx[info.ctx_registration_variable] then
		--We've never run this counter before, set it to 1
		ctx[info.ctx_registration_variable] = 1
	else
		--We've run this counter before, increment value
		local num_times = ctx[info.ctx_registration_variable]
		ctx[info.ctx_registration_variable] = num_times + 1
	end

	--If this counter has run enough times, trigger the next encounter
	if ctx[info.ctx_registration_variable] >= info.times then
		self._sv.resolved_out_edge = info.out_edges.success
	else 
		self._sv.resolved_out_edge = info.out_edges.fail
	end
	ctx.arc:trigger_next_encounter(ctx)
end

function CounterEncounter:_should_progress_encounter(ctx, info)
	local needs_specified_entity
	local reference_entity
	if info.optional_reference_entity then
		needs_specified_entity = true
		local entity = ctx:get(info.optional_reference_entity)
		if radiant.util.is_a(entity, Entity) and entity:is_valid() then
			reference_entity = entity
		end
	end
	return not needs_specified_entity or (needs_specified_entity and reference_entity)
end

function CounterEncounter:stop()
end

return CounterEncounter