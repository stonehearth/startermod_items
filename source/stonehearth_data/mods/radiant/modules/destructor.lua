-- class Destructor
--
-- A utility class which makes it really easy to implement objects which implement
-- only a single "destroy" method.

local function _destroy(self)
	local result
	if self._destroy_cb then
		result = self._destroy_cb()
		self._destroy_cb = nil
	end
	return result
end

local function _new(destroy_cb)
	assert(destroy_cb)
	local dtor = {
		_destroy_cb = destroy_cb,
		destroy = _destroy,
	}
	return dtor
end

local Destructor = {
	new = _new,
}

return Destructor
