local SnaredBuffController = class()

function SnaredBuffController:__init(entity, buff)
   self._entity = entity
   self._buff = buff

   -- do nothing, this is just to demonstrate the pattern for now
end

return SnaredBuffController
