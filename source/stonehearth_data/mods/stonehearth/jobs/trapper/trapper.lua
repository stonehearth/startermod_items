local TrapperClass = class() 

function TrapperClass:initialize(entity)
   self._sv._entity = entity
   self._sv.last_gained_lv = 0
end

function TrapperClass:promote(json)
end

function TrapperClass:demote()
end

return TrapperClass
