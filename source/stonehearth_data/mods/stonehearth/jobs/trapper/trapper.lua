local TrapperClass = class() 

function TrapperClass:initialize(entity)
   self._sv._entity = entity
   self._sv.last_gained_lv = 0
   self._sv.is_current_class = false
   self._sv.job_data = nil

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
   self._sv.is_current_class = true
   self._sv.job_data = json
   self._sv.job_name = json.name

   if json.xp_rewards then
      self._sv.xp_rewards = json.xp_rewards
   end
   if json.level_data then
      self._sv.level_data = json.level_data
   end

   self:_create_xp_listeners()
   self.__saved_variables:mark_changed()
end

-- If there is a job title for this level, return it. 
function TrapperClass:get_job_title()
   local lv_data = self._sv.level_data[tostring(self._sv.last_gained_lv)]
   if not lv_data then return nil end

   if lv_data and lv_data.title then
      return lv_data.title
   end

   return nil
end

function TrapperClass:get_job_level()
   return self._sv.last_gained_lv
end

function TrapperClass:_create_xp_listeners()
   self._clear_trap_listener = radiant.events.listen(self._sv._entity, 'stonehearth:clear_trap', self, self._on_clear_trap)
   self._befriend_pet_listener = radiant.events.listen(self._sv._entity, 'stonehearth:befriend_pet', self, self._on_pet_befriended)
end

function TrapperClass:_remove_xp_listeners()
   self._clear_trap_listener:destroy()
   self._clear_trap_listener = nil

   self._befriend_pet_listener:destroy()
   self._befriend_pet_listener = nil
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

-- Called by the job component to do job-specific level up
-- Increment our levels in this class by 1
-- Given the number of the new level, see if there are any perks that should be applied
-- TODO: Is there ever a case where perks are taken away?
-- returns: info about the new level 
function TrapperClass:level_up()
   self._sv.last_gained_lv = self._sv.last_gained_lv + 1
   local job_updates_for_level = self._sv.level_data[tostring(self._sv.last_gained_lv)]

   --If there is NO data for the class at this level, everything stays the same
   if not job_updates_for_level then
      return
   end

   --Apply each perk (there probably is only one)
   local perk_descriptions = {}
   for i, perk_data in ipairs(job_updates_for_level.perks) do

      self[perk_data.type](self, perk_data)

      --Collect text about the perk
      local perk_info = {perk_name = perk_data.perk_name, description = perk_data.description}
      table.insert(perk_descriptions, perk_info)
   end

   local level_data = {
      new_level = self._sv.last_gained_lv, 
      job_name = self._sv.job_name,
      descriptions = perk_descriptions
   }

   self.__saved_variables:mark_changed()
   
   return level_data
end

-- Add the buff described in the buff_name
function TrapperClass:apply_buff(args)
   radiant.entities.add_buff(self._sv._entity, args.buff_name)   
end

--Increase the size of the backpack
function TrapperClass:increase_backpack_size(args)
   local backpack_component = self._sv._entity:add_component('stonehearth:backpack')
   backpack_component:change_max_capacity(args.backpack_size_increase)
end

function TrapperClass:demote()
   self:_remove_xp_listeners()
   self._sv.is_current_class = false

   self.__saved_variables:mark_changed()
end

return TrapperClass
