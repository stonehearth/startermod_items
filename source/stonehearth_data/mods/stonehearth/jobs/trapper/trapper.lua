local TrapperClass = class() 

function TrapperClass:initialize(entity)
   self._sv._entity = entity
   self._sv.last_gained_lv = 0
   self._sv.is_current_class = false

   self:restore()
end

function TrapperClass:restore()
   --Always do these things
   self._job_component = self._sv._entity:get_component('stonehearth:job')

   --If we load and we're the current class, do these things
   if self._sv.is_current_class then
      self:_create_xp_listeners()
   end
end

function TrapperClass:promote(json)
   self._sv.is_current_class = true

   if json and json.xp_rewards then
      self._sv.xp_rewards = json.xp_rewards
   end
   if json and json.level_data then
      self._sv.level_data = json.level_data
   end

   self:_create_xp_listeners()
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
function TrapperClass:level_up()
   self._sv.last_gained_lv = self._sv.last_gained_lv + 1
   local job_updates_for_level = self._sv.level_data[tostring(self._sv.last_gained_lv)]

   --If there is NO data for the class at this level, everything stays the same
   if not job_updates_for_level then
      return
   end

   --TODO: Change the title if there is a new title

   --Apply each perk (there probably is only one)
   for i, perk_data in ipairs(job_updates_for_level.perks) do
      if perk_data.type == 'buff' then
         self:_apply_buff(perk_data.buff_name)
      elseif perk_data.type == 'function' then
         self:_call_function(perk_data.fn_name, perk_data.file, perk_data.args)
      end
   end

end

function TrapperClass:_apply_buff(buff_name)
   radiant.entities.add_buff(self._sv._entity, buff_name)   
end

-- Call the function named in the arguments
-- @param file: name of the file that contains the fn. If the name is 'default' it means this file
function TrapperClass:_call_function(fn_name, file, args)
   if file == 'default' then
      --args.entity = self._sv._entity
      self[fn_name](self._sv._entity, args)
   end
end

--These dynamically generated functions!
function TrapperClass.increase_backpack_size(entity, args)
   local backpack_component = entity:add_component('stonehearth:backpack')
   backpack_component:change_max_capacity(args.backpack_size_increase)
end

function TrapperClass:demote()
   self:_remove_xp_listeners()
   self._sv.is_current_class = false
end

return TrapperClass
