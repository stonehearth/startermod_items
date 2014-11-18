local TrapperClass = class() 
local job_helper = require 'jobs.job_helper'

function TrapperClass:initialize(entity)
   job_helper.initialize(self._sv, entity)
   self:restore()
end

function TrapperClass:restore()
   --Always do these things
   self._job_component = self._sv._entity:get_component('stonehearth:job')

   --If we load and we're the current class, do these things
   if self._sv.is_current_class then
      self:_create_xp_listeners()
   end

   self.__saved_variables:mark_changed()
end

function TrapperClass:promote(json)
   job_helper.promote(self._sv, json)

   self:_create_xp_listeners()
   self.__saved_variables:mark_changed()
end

-- Returns the level the character has in this class
function TrapperClass:get_job_level()
   return self._sv.last_gained_lv
end

-- Returns whether we're at max level.
-- NOTE: If max level is nto declared, always false
function TrapperClass:is_max_level()
   return self._sv.is_max_level 
end

-- Returns all the data for all the levels
function TrapperClass:get_level_data()
   return self._sv.level_data
end

-- We keep an index of perks we've unlocked for easy lookup
function TrapperClass:unlock_perk(id)
   self._sv.attained_perks[id] = true
   self.__saved_variables:mark_changed()
end

-- Given the ID of a perk, find out if we have the perk. 
function TrapperClass:has_perk(id)
   return self._sv.attained_perks[id]
end

-- Called by the job component to do job-specific level up
-- Increment our levels in this class by 1
function TrapperClass:level_up()
   job_helper.level_up(self._sv)

   self.__saved_variables:mark_changed()
end

--Call when it's time to demote
function TrapperClass:demote()
   self:_remove_xp_listeners()
   self._sv.is_current_class = false

   self.__saved_variables:mark_changed()
end

--Private functions

function TrapperClass:_create_xp_listeners()
   self._clear_trap_listener = radiant.events.listen(self._sv._entity, 'stonehearth:clear_trap', self, self._on_clear_trap)
   self._befriend_pet_listener = radiant.events.listen(self._sv._entity, 'stonehearth:befriend_pet', self, self._on_pet_befriended)

   --Move into another function that is activated by a test
   self._set_trap_listener = radiant.events.listen(self._sv._entity, 'stonehearth:set_trap', self, self._on_set_trap)
end

function TrapperClass:_remove_xp_listeners()
   self._clear_trap_listener:destroy()
   self._clear_trap_listener = nil

   self._befriend_pet_listener:destroy()
   self._befriend_pet_listener = nil
   
   self._set_trap_listener:destroy()
   self._set_trap_listener = nil
end

-- Called if the trapper is harvesting a trap for food. 
-- @param args - the trapped_entity_id field inside args is nil if there is no critter, and true if there is a critter 
function TrapperClass:_on_clear_trap(args)
   if args.trapped_entity_id then
      self._job_component:add_exp(self._sv.xp_rewards['successful_trap'])
   else
      self._job_component:add_exp(self._sv.xp_rewards['unsuccessful_trap'])
   end
end

-- Called when the trapper is befriending a pet
-- @param args - the pet_id field inside args is nil if there is no critter, and the ID if there is a critter 
function TrapperClass:_on_pet_befriended(args)
   if args.pet_id then
      self._job_component:add_exp(self._sv.xp_rewards['befriend_pet'])
   end
end

-- We actually want the XP to be gained on harvesting; this is mostly for testing purposes.
function TrapperClass:_on_set_trap(args)
   --Comment in for testing, or write activation fn for autotests
   self._job_component:add_exp(90)
end


-- Functions for level up

-- Add the buff described in the buff_name
function TrapperClass:apply_buff(args)
   radiant.entities.add_buff(self._sv._entity, args.buff_name)   
end

-- Add the equipment specified in the description
-- Save the equipment so we can remove it later.
function TrapperClass:add_equipment(args)
   self._sv[args.equipment] = radiant.entities.create_entity(args.equipment)
   radiant.entities.equip_item(self._sv._entity, self._sv[args.equipment])
end

--Increase the size of the backpack
function TrapperClass:increase_backpack_size(args)
   local backpack_component = self._sv._entity:add_component('stonehearth:backpack')
   backpack_component:change_max_capacity(args.backpack_size_increase)
end

-- Functions for demote

--Remove any buffs added
function TrapperClass:remove_buff(args)
   radiant.entities.remove_buff(self._sv._entity, args.buff_name)
end

--If the equipment specified is stored in us, remove it
function TrapperClass:remove_equipment(args)
   if self._sv[args.equipment] then
      radiant.entities.unequip_item(self._sv._entity, self._sv[args.equipment])
      self._sv[args.equipment] = nil
   end
end

--Make the backpack size smaller
function TrapperClass:decrease_backpack_size(args)
   local backpack_component = self._sv._entity:add_component('stonehearth:backpack')
   backpack_component:change_max_capacity(args.backpack_size_increase * -1)
end

--Call when it's time to demote
function TrapperClass:demote()
   self:_remove_xp_listeners()
   self._sv.is_current_class = false

   self.__saved_variables:mark_changed()
end

return TrapperClass
