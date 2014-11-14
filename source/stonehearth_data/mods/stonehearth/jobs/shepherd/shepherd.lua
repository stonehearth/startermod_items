local ShepherdClass = class()
local job_helper = require 'jobs.job_helper'

--- Public functions, required for all classes

function ShepherdClass:initialize(entity)
   job_helper.initialize(self._sv, entity)
   self:restore()
end

function ShepherdClass:restore()
   self._job_component = self._sv._entity:get_component('stonehearth:job')

   --If we load and we're the current class, do these things
   if self._sv.is_current_class then
      --self:_create_xp_listeners()
   end

   self.__saved_variables:mark_changed()
end

function ShepherdClass:promote(json)
   job_helper.promote(self._sv, json)
   --self:_create_xp_listeners()

   self.__saved_variables:mark_changed()
end

-- Returns the level the character has in this class
function ShepherdClass:get_job_level()
   return self._sv.last_gained_lv
end

-- Returns whether we're at max level.
-- NOTE: If max level is nto declared, always false
function ShepherdClass:is_max_level()
   return self._sv.is_max_level 
end

-- Returns all the data for all the levels
function ShepherdClass:get_level_data()
   return self._sv.level_data
end

-- We keep an index of perks we've unlocked for easy lookup
function ShepherdClass:unlock_perk(id)
   self._sv.attained_perks[id] = true
   self.__saved_variables:mark_changed()
end

-- Given the ID of a perk, find out if we have the perk. 
function ShepherdClass:has_perk(id)
   return self._sv.attained_perks[id]
end

-- Call when it's time to level up in this class
function ShepherdClass:level_up()
   job_helper.level_up(self._sv)
   self.__saved_variables:mark_changed()
end

function ShepherdClass:demote()
   --self:_remove_xp_listeners()
   self._sv.is_current_class = false
   self.__saved_variables:mark_changed()
end

-- Private Functions

return ShepherdClass
