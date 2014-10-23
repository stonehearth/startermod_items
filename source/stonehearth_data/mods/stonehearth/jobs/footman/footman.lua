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

--Right now, the footman only gains XP when he's hit someone for damage. 
--We do log attacks though, for debugging purposes. 
function FootmanClass:_create_xp_listeners()
   self._on_attack = radiant.events.listen(self._sv._entity, 'stonehearth:combat:begin_melee_attack', self, self._on_attack)
   self._on_damage_dealt = radiant.events.listen(self._sv._entity, 'stonehearth:combat:melee_attack_connect', self, self._on_damage_dealt)
end

function FootmanClass:_remove_xp_listeners()
   self._on_attack:destroy()
   self._on_attack = nil

   self._on_damage_dealt:destroy()
   self._on_damage_dealt = nil
end

function FootmanClass:_on_attack(args)
   --For debug purposes   
   --self._job_component:add_exp(100)
end

-- Whenever you do damage, you get XP equal to the target's menace
function FootmanClass:_on_damage_dealt(args)
   local exp = 0
   if not args.target_exp then
      exp = self._sv.xp_rewards['base_exp_per_hit']
   else
      exp = args.target_exp
   end
   self._job_component:add_exp(exp)
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

--Add or remove a type of combat action the footman can choose in melee
function FootmanClass:add_combat_action(args)
   job_helper.add_equipment(self._sv, args)
   local combat_state = stonehearth.combat:get_combat_state(self._sv._entity)
   combat_state:recompile_combat_actions(args.action_type)
end

function FootmanClass:remove_combat_action(args)
   job_helper.remove_equipment(self._sv, args)
   local combat_state = stonehearth.combat:get_combat_state(self._sv._entity)
   combat_state:recompile_combat_actions(args.action_type)
end

return FootmanClass