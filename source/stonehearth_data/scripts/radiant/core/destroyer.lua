local Destroyer = class()
function Destroyer:__init(fn)
   self._cb = fn
end

function Destroyer:destroy()
   if self._cb then
      self._cb()
      self._cb = nil
   end
end

return Destroyer
