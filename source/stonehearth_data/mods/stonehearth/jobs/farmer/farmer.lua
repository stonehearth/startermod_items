local FarmerClass = class()

function FarmerClass:initialize(entity)
   self._sv.entity = entity
   self._sv.last_gained_lv = 0
end

function FarmerClass:promote()
end

function FarmerClass:restore()
end

function FarmerClass:demote()
end

return FarmerClass
