local LadderBuilderDestructor = radiant.class()

function LadderBuilderDestructor:initialize(ladder_builder, to)
   checks('self', '?controller', '?Point3')
   self._sv.is_server = radiant.is_server
   self._sv.ladder_builder = ladder_builder
   self._sv.to = to
end

function LadderBuilderDestructor:destroy()
   -- this is awkward... all controllers get remoted to the client.  make sure we
   -- don't invoke the binding there when the LadderBuilderDestructor is reaped.  the correct
   -- way to fix this might be to just not remote these objects to the client.
   -- revisit when we have to develop that tech for multiplayer. - tony
   if self._sv.is_server ~= radiant.is_server then
      return
   end
   if self._sv.ladder_builder then
      self._sv.ladder_builder:remove_point(self._sv.to)
      self._sv.ladder_builder = nil
   end
end

function LadderBuilderDestructor:get_id()
   if self._sv.ladder_builder then
      return self._sv.ladder_builder:get_id()
   end
   return -1
end

return LadderBuilderDestructor
