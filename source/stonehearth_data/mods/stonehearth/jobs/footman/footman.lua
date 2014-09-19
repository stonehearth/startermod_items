local FootmanClass = class()

function FootmanClass:initialize(entity)
   self._sv._entity = entity
   self._sv.last_gained_lv = 0
end

function FootmanClass:promote(json)
end

function FootmanClass:demote()
   -- TODO: unequip the weapon
   assert(false, 'not implemented')
end

return FootmanClass