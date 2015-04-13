-- class Destructor
--
-- A utility class which makes it really easy to implement objects which implement
-- only a single "destroy" method.

local Destructor = radiant.class()

function Destructor:initialize(binding)
   checks('self', 'binding')
   self._sv.binding = binding
   self._sv.is_server = radiant.is_server
end

function Destructor:destroy()
   -- this is awkward... all controllers get remoted to the client.  make sure we
   -- don't invoke the binding there when the destructor is reaped.  the correct
   -- way to fix this might be to just not remote these objects to the client.
   -- revisit when we have to develop that tech for multiplayer. - tony
   if self._sv.is_server ~= radiant.is_server then
      return
   end
   local results = {}
   local binding = self._sv.binding
   if binding then
      self._sv.binding = nil
      results = { radiant.invoke(binding) }
   end
   return unpack(results)
end

return Destructor
