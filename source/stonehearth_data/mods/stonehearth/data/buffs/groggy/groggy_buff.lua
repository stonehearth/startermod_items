local GroggyBuffController = class()

function GroggyBuffController:__init(entity, buff)
   self._entity = entity
   self._buff = buff

   -- do nothing, this is just to demonstrate the pattern for now
end

return GroggyBuffController
