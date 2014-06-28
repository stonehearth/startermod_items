
-- class Destructor
--
-- A utility class which makes it really easy to implement objects which implement
-- only a single "destroy" method.
--
local Destructor = class()

-- create a new destructor. `destroy_cb` will be called the first time the user
-- calls :destroy() on this object
--
--    @param destroy_cb - the function to call on :destroy()
function Destructor:__init(destroy_cb)
   assert(destroy_cb)
   self._destroy_cb = destroy_cb
end

-- Calls the registered `destroy_cb`
--
function Destructor:destroy()
   if self._destroy_cb then
      self._destroy_cb()
      self._destroy_cb = nil
   end
end

return Destructor
