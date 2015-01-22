local FarmerClass = class()
local job_helper = require 'jobs.job_helper'

--- Public functions, required for all classes

function FarmerClass:initialize(entity)
   job_helper.initialize(self._sv, entity)
   self:restore()
end

function FarmerClass:restore()
   self._job_component = self._sv._entity:get_component('stonehearth:job')

   --If we load and we're the current class, do these things
   if self._sv.is_current_class then
      self:_create_xp_listeners()
   end

   self.__saved_variables:mark_changed()
end

function FarmerClass:promote(json)
   job_helper.promote(self._sv, json)
   self:_create_xp_listeners()

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

-- Returns all the data for all the levels
function FarmerClass:get_level_data()
   return self._sv.level_data
end

-- We keep an index of perks we've unlocked for easy lookup
function FarmerClass:unlock_perk(id)
   self._sv.attained_perks[id] = true
   self.__saved_variables:mark_changed()
end

-- Given the ID of a perk, find out if we have the perk. 
function FarmerClass:has_perk(id)
   return self._sv.attained_perks[id]
end

--Returns true if this class participates in worker defense. False otherwise.
--Unless the class specifies in their _description, it's true by default.
function FarmerClass:get_worker_defense_participation()
   return self._sv.worker_defense_participant
end

-- Call when it's time to level up in this class
function FarmerClass:level_up()
   job_helper.level_up(self._sv)
   self.__saved_variables:mark_changed()
end

function FarmerClass:demote()
   self:_remove_xp_listeners()
   self._sv.is_current_class = false
   self.__saved_variables:mark_changed()
end

-- Private Functions

-- Farmers gain XP when they harvest things. The amount of XP depends on the crop
function FarmerClass:_create_xp_listeners()
   self._on_harvest_listener = radiant.events.listen(self._sv._entity, 'stonehearth:harvest_crop', self, self._on_harvest)
end

function FarmerClass:_remove_xp_listeners()
   self._on_harvest_listener:destroy()
   self._on_harvest_listener = nil
end

function FarmerClass:_on_harvest(args)
   local crop = args.crop_uri
   local xp_to_add = self._sv.xp_rewards["base_exp_per_harvest"]
   if self._sv.xp_rewards[crop] then
      xp_to_add = self._sv.xp_rewards[crop] 
   end
   self._job_component:add_exp(xp_to_add)
end

function FarmerClass:apply_buff(args)
   radiant.entities.add_buff(self._sv._entity, args.buff_name)   
end

function FarmerClass:remove_buff(args)
   radiant.entities.remove_buff(self._sv._entity, args.buff_name)
end

return FarmerClass
