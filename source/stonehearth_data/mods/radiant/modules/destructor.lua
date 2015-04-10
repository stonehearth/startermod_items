-- class Destructor
--
-- A utility class which makes it really easy to implement objects which implement
-- only a single "destroy" method.

local Destructor = radiant.class()

function Destructor:__init(destroy_cb)
	assert(destroy_cb)
	self._destroy_cb = destroy_cb
end

function Destructor:destroy()
	local result
	if self._destroy_cb then
		result = self._destroy_cb()
		self._destroy_cb = nil
	end
	return result
end

return Destructor
