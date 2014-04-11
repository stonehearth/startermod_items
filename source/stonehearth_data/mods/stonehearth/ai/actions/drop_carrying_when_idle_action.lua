local DropCarryingWhenIdle = class()

DropCarryingWhenIdle.name = 'drop carrying when idle'
DropCarryingWhenIdle.does = 'stonehearth:idle'
DropCarryingWhenIdle.args = { }
DropCarryingWhenIdle.version = 2
DropCarryingWhenIdle.priority = 2

function DropCarryingWhenIdle:start_thinking(ai, entity, args)
	-- there might not be another action that wants to do anything with us while
	-- we're carrying this thing, so drop it after a bit
	self._timer = stonehearth.calendar:set_timer('5s', function()
			ai:set_think_output()
		end)
end

function DropCarryingWhenIdle:stop_thinking(ai, entity, args)
	if self._timer then
		self._timer:destroy()
		self._timer = nil
	end
end

local ai = stonehearth.ai
return ai:create_compound_action(DropCarryingWhenIdle)
		:when(function(ai) return ai.CURRENT.carrying ~= nil end)
        :execute('stonehearth:wander_within_leash', { radius = 1, radius_min = 3 })
        :execute('stonehearth:drop_carrying_now', {})
