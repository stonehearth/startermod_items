local FootmanClass = class()
local job_helper = require 'jobs.job_helper'

--- Public functions, required for all classes

function FootmanClass:initialize(entity)
   job_helper.initialize(self._sv, entity)
   self:restore()
end

--Always do these things
function FootmanClass:restore()
   self._job_component = self._sv._entity:get_component('stonehearth:job')

   --If we load and we're the current class, do these things
   if self._sv.is_current_class then
      self:_create_xp_listeners()
   end

   self.__saved_variables:mark_changed()
end

-- Call when it's time to promote someone to this class
function FootmanClass:promote(json)
   job_helper.promote(self._sv, json)

   self:_create_xp_listeners()
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

-- Returns all the data for all the levels
function FootmanClass:get_level_data()
   return self._sv.level_data
end

-- We keep an index of perks we've unlocked for easy lookup
function FootmanClass:unlock_perk(perk_id)
   self._sv.attained_perks[perk_id] = true
   self.__saved_variables:mark_changed()
end

-- Call when it's time to level up in this class
function FootmanClass:level_up()
   job_helper.level_up(self._sv)
   self.__saved_variables:mark_changed()
end

-- Call when it's time to demote someone from this class
function FootmanClass:demote()
   self:_remove_xp_listeners()
   self._sv.is_current_class = false
   self.__saved_variables:mark_changed()
end

--- Private functions

function FootmanClass:_create_xp_listeners()

   self._on_attack = radiant.events.listen(self._sv._entity, 'stonehearth:combat:begin_melee_attack', self, self._on_attack)
   self._on_defeat_enemy = radiant.events.listen(self._sv._entity, 'stonehearth:defeat_enemy', self, self._on_defeat_enemy)
   self._on_patrol = radiant.events.listen(self._sv._entity, 'stonehearth:on_patrol', self, self._on_patrol)
end

function FootmanClass:_remove_xp_listeners()
   self._on_attack:destroy()
   self._on_attack = nil
   
   self._on_defeat_enemy:destroy()
   self._on_defeat_enemy = nil

   self._on_patrol:destroy()
   self._on_patrol = nil
end

function FootmanClass:_on_attack(args)
   self._job_component:add_exp(self._sv.xp_rewards['attack'])
end

function FootmanClass:_on_defeat_enemy(args)
end

function FootmanClass:_on_patrol(args)
end

--- Functions specific to level up
function FootmanClass:apply_chained_buff(args)
   radiant.entities.remove_buff(self._sv._entity, args.last_buff)
   radiant.entities.add_buff(self._sv._entity, args.buff_name)   
end

function FootmanClass:apply_buff(args)
   radiant.entities.add_buff(self._sv._entity, args.buff_name)   
end

function FootmanClass:remove_buff(args)
   radiant.entities.remove_buff(self._sv._entity, args.buff_name)
end

return FootmanClass