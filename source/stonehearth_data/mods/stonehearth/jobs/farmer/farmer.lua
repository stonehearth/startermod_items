local FarmerClass = class()

function FarmerClass:initialize(entity)
   self._sv.entity = entity
   self._sv.last_gained_lv = 0
   self._sv.is_current_class = false
   self._sv.is_max_level = false
end

function FarmerClass:restore()
end

function FarmerClass:promote(json)
   self._sv.is_current_class = true
   self._sv.job_name = json.name

   self.__saved_variables:mark_changed()
end

-- Returns the level the character has in this class
function FarmerClass:get_job_level()
   return self._sv.last_gained_lv
end

-- Returns whether we're at max level.
-- NOTE: If max level is nto declared, always false
function FarmerClass:is_max_level()
   return self._sv.is_max_level 
end

function FarmerClass:demote()
   self._sv.is_current_class = false
   self.__saved_variables:mark_changed()
end

return FarmerClass
