local Point3 = _radiant.csg.Point3

local WorkerClass = class()
local job_helper = require 'jobs.job_helper'

--[[
   A controller that manages all the relevant data about the worker class
   Workers don't level up, so this is all that's needed.
]]

function WorkerClass:initialize(entity)
   job_helper.initialize(self._sv, entity)
   self._sv.no_levels = true
end

function WorkerClass:restore()
end

function WorkerClass:promote(json, options)
   job_helper.promote(self._sv, json)

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

-- Returns all the data for all the levels
function WorkerClass:get_level_data()
   return nil
end

-- Given the ID of a perk, find out if we have the perk. 
function WorkerClass:has_perk(id)
   return false
end

--Returns true if this class participates in worker defense. False otherwise.
--Unless the class specifies in their _description, it's true by default.
function WorkerClass:get_worker_defense_participation()
   return self._sv.worker_defense_participant
end

function WorkerClass:demote()
   self._sv.is_current_class = false
   self.__saved_variables:mark_changed()
end

return WorkerClass
