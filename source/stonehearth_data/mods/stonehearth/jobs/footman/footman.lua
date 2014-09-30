local FootmanClass = class()

function FootmanClass:initialize(entity)
   self._sv._entity = entity
   self._sv.last_gained_lv = 0
   self._sv.is_current_class = false
   self._sv.is_max_level = false
end

function FootmanClass:promote(json)
   self._sv.is_current_class = true
   self._sv.job_name = json.name

   self.__saved_variables:mark_changed()
end

-- Returns the level the character has in this class
function FootmanClass:get_job_level()
   return self._sv.last_gained_lv
end

-- Returns whether we're at max level.
-- NOTE: If max level is nto declared, always false
function FootmanClass:is_max_level()
   return self._sv.is_max_level 
end


function FootmanClass:demote()

   -- TODO: unequip the weapon
   assert(false, 'not implemented')

   self._sv.is_current_class = false
   self.__saved_variables:mark_changed()
end

return FootmanClass