-- NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE

-- Temporarily disabled until Tony can figure out why this doesn't work. =..(

-- NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE

local DropCarryingWhenIdle = class()

DropCarryingWhenIdle.name = 'drop carrying when idle'
DropCarryingWhenIdle.does = 'stonehearth:idle'
DropCarryingWhenIdle.args = { }
DropCarryingWhenIdle.version = 2
DropCarryingWhenIdle.priority = 2

function DropCarryingWhenIdle:start_thinking(ai, entity, args)
	-- there might not be another action that wants to do anything with us while
	-- we're carrying this thing, so drop it after a bit.  we use a realtime timer,
	-- to give the processor time to do "something" with whatever we're carrying
	-- before giving up (e.g. put it in a stockpile, build more wall with it, etc.)	
	self._timer = radiant.set_realtime_timer(2000, function()
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
		:when(function(ai) 
			return ai.CURRENT.carrying ~= nil end)
        :execute('stonehearth:wander_within_leash', { radius = 3 })
        :execute('stonehearth:drop_carrying_now', {})
