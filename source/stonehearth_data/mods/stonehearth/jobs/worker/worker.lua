local Point3 = _radiant.csg.Point3

local WorkerClass = class()

--[[
   A controller that manages all the relevant data about the worker class
   Workers don't level up, so this is all that's needed.
]]

function WorkerClass:initialize(entity)
   self._sv.entity = entity
   self._sv.last_gained_lv = 0
   self._sv.is_current_class = false
   self._sv.no_levels = true
end

function WorkerClass:restore()
end

function WorkerClass:promote(json)
   self._sv.is_current_class = true

   if json.level_data then
      self._sv.level_data = json.level_data
   end

   self.__saved_variables:mark_changed()
end

-- Returns the level the character has in this class
function WorkerClass:get_job_level()
   return self._sv.last_gained_lv
end

-- Returns whether we're at max level.
-- NOTE: If max level is nto declared, returns false
function WorkerClass:is_max_level()
   return self._sv.is_max_level 
end

function WorkerClass:demote()
   self._sv.is_current_class = false
   self.__saved_variables:mark_changed()
end

return WorkerClass
