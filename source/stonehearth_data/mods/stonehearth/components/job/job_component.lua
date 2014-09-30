--[[
   This component stores data about the job that a settler has.
]]
local rng = _radiant.csg.get_default_rng()

local JobComponent = class()

function JobComponent:initialize(entity, json)
   self._entity = entity

   self._sv = self.__saved_variables:get_data()

   if not self._sv._initialized then
      self._sv._initialized = true
      self._sv.job_controllers = {}
      if json.xp_equation_for_next_level then
         self._sv.xp_equation = json.xp_equation_for_next_level
      end
      if json.default_level_announcement then
         self._sv._default_level_announcement = json.default_level_announcement
      end
      if json.starting_level_title then
         self._sv._starting_level_title = json.starting_level_title
      end
      if json.default_level_title then
         self._sv._default_level_title = json.default_level_title
      end
   end

   if self._sv.job_uri then
      radiant.events.listen(entity, 'radiant:entity:post_create', function(e)
            self._job_json = radiant.resources.load_json(self._sv.job_uri, true)

            if self._job_json then
               --Add self back to the right task groups
               if self._job_json.task_groups then
                  self:_add_to_task_groups(self._job_json.task_groups)
               end
            end
            return radiant.events.UNLISTEN
         end)
   end
   self._kill_listener = radiant.events.listen(self._entity, 'stonehearth:kill_event', self, self._on_kill_event)
end

function JobComponent:destroy()
   self._kill_listener:destroy()
   self._kill_listener = nil
end

--When we've been killed, dump our talisman on the ground
function JobComponent:_on_kill_event()
   if self._sv.talisman_uri then
      local location = radiant.entities.get_world_grid_location(self._entity)
      local player_id = radiant.entities.get_player_id(self._entity)
      local output_table = {}
      output_table[self._sv.talisman_uri] = 1
      --TODO: is it possible that this gets dumped onto an inaccessible location?
      radiant.entities.spawn_items(output_table, location, 1, 2, player_id)
   end
end

function JobComponent:get_job_uri()
   return self._sv.job_uri
end

function JobComponent:get_roles()
   return self._job_json.roles or ''
end

-- Figure out the job title for the current job (level + job name)
-- First try to get it from the job data
-- If that doesn't work, compose it from the default
function JobComponent:_get_current_job_title(job_json)
   local new_title = self:_get_title_for_level(job_json)
   if not new_title then
      new_title = self:_compose_default_job_name(
         job_json,
         self._sv.curr_job_controller:get_job_level(), 
         job_json.name)
   end
   return new_title
end

-- Look up the level of the character in this job and
-- see if there is a specific title for that level
-- if so, return it, if not, return nil. 
function JobComponent:_get_title_for_level(job_json)
   local curr_level = self._sv.curr_job_controller:get_job_level()

   local lv_data = job_json.level_data[tostring(curr_level)]
   if not lv_data then return nil end

   if lv_data and lv_data.title then
      return lv_data.title
   end

   return nil
end

-- Composes the detailed job title if none is provided by the class. 
function JobComponent:_compose_default_job_name(job_json, level, job_name)
   if level == 0 then
      local apprentice_string = self._sv._starting_level_title
      apprentice_string = string.gsub(apprentice_string, '__job_name__', job_name)
      return apprentice_string
   else
      local full_name_string = self._sv._default_level_title
      full_name_string = string.gsub(full_name_string, '__level_number__', level)
      full_name_string = string.gsub(full_name_string, '__job_name__', job_name)
      return full_name_string
   end
end

function JobComponent:promote_to(job_uri)
   self._job_json = radiant.resources.load_json(job_uri, true)
   if self._job_json then
      self:demote()
      self._sv.job_uri = job_uri
      self._sv.talisman_uri = self._job_json.talisman_uri
      self._sv.class_icon = self._job_json.icon

      --Strangely, doesn't exist yet when this is called in init, creates duplicate component!
      if not self._sv.attributes_component then
         self._sv.attributes_component = self._entity:get_component('stonehearth:attributes')
      end

      --Whenever you get a new job, dump all the xp that you've accured so far to your next level
      self._sv.current_level_exp = 0
      self._sv.xp_to_next_lv = self:_calculate_xp_to_next_lv()

      self:_equip_outfit(self._job_json)
      self._sv._default_stance = self._job_json.default_stance
      self:reset_to_default_comabat_stance()

      --Create the job controller, if we don't yet have one
      if not self._sv.job_controllers[self._sv.job_uri] then
         --create the controller
         self._sv.job_controllers[self._sv.job_uri] = 
            radiant.create_controller(self._job_json.controller, self._entity)
      end
      self._sv.curr_job_controller = self._sv.job_controllers[self._sv.job_uri]
      self._sv.curr_job_controller:promote(self._job_json, self._sv.talisman_uri)
      self._sv.curr_job_name = self:_get_current_job_title(self._job_json)
      self:_set_unit_info(self._sv.curr_job_name)
      
      --Add self to task groups
      if self._job_json.task_groups then
         self:_add_to_task_groups(self._job_json.task_groups)
      end
      -- so good!  keep this one, lose the top one.  too much "collusion" between components =)
      radiant.events.trigger(self._entity, 'stonehearth:job_changed', { entity = self._entity })
      self.__saved_variables:mark_changed()
   end
end

function JobComponent:demote()
   self:_remove_outfit()

   if self._sv.curr_job_controller then
      self._sv.curr_job_controller:demote()
      self._sv.curr_job_controller = nil
   end

   self._sv._default_stance = 'passive'
   self:reset_to_default_comabat_stance()

   if self._job_json and self._job_json.task_groups then
      self:_remove_from_task_groups(self._job_json.task_groups)
   end

   self.__saved_variables:mark_changed()
end

-- Called from the job scripts
-- When XP exceeds XP to next level, call level up
-- If you gain a TON of XP, call level up an appropriate # of times
-- If there is leftover XP after the level(s), that should be the new amount of XP
-- that is stored in self._sv.current_level_exp
function JobComponent:add_exp(value)
   if self._sv.curr_job_controller:is_max_level() then
      return
   end

   self._sv.current_level_exp = self._sv.current_level_exp + value
   local original_level = self._sv.attributes_component:get_attribute('total_level')

   while self._sv.current_level_exp >= self._sv.xp_to_next_lv do
      self._sv.current_level_exp = self._sv.current_level_exp - self._sv.xp_to_next_lv
      self:_level_up()
   end

   self.__saved_variables:mark_changed()   
end

-- Called by add_exp. Calls the profession-specific job controller to tell it to level up
function JobComponent:_level_up()
   --Change all the attributes to the new level
   --Should the general HP increase (class independent) be reflected as a permanent buff or a quiet stat increase?
   local curr_level = self._sv.attributes_component:get_attribute('total_level')
   self._sv.attributes_component:set_attribute('total_level', curr_level + 1)
   
   --Add all the universal level dependent buffs/bonuses, etc

   local optional_description = self._sv.curr_job_controller:level_up()
   self._sv.curr_job_name = self:_get_current_job_title(self._job_json)
   self:_set_unit_info(self._sv.curr_job_name)   

   if optional_description then
      local player_id = radiant.entities.get_player_id(self._entity)
      local name = radiant.entities.get_display_name(self._entity)
      local title = self._sv._default_level_announcement
      title = string.gsub(title, '__name__', name)
      title = string.gsub(title, '__job_name__', optional_description.job_name)
      title = string.gsub(title, '__level_number__', optional_description.new_level)
      
      stonehearth.bulletin_board:post_bulletin(player_id)
         :set_callback_instance(self)
         :set_data({
            title = title, 
            zoom_to_entity = self._entity
         })

      --Trigger an event so people can extend the class system
      radiant.events.trigger_async(self._entity, 'stonehearth:level_up', {
         level = optional_description.new_level, 
         job_uri = self._sv.job_uri, 
         job_name = self._sv.curr_job_name })
   end

   radiant.effects.run_effect(self._entity, '/stonehearth/data/effects/level_up')
   
   --TODO: localize, decide how we want to announce this
   --TODO: update UI/char sheet, etc
   --TODO: have this affect happiness!
   --local name = radiant.entities.get_display_name(self._entity)
   --stonehearth.events:add_entry(name .. ' has leveled up in ' .. self._sv.curr_job_name .. '!')
   self._sv.xp_to_next_lv = self:_calculate_xp_to_next_lv()
   self.__saved_variables:mark_changed()   
end

-- Calculate the exp requried to get to the level after this one. 
function JobComponent:_calculate_xp_to_next_lv()
   local next_level = self._sv.attributes_component:get_attribute('total_level') + 1
   local equation = string.gsub(self._sv.xp_equation, 'next_level', next_level)
   local fn = loadstring('return ' .. equation)
   return fn()
end

--- Adds this person to a set of task groups
--  @param task_groups - an array of the task groups this person should belong to
function JobComponent:_add_to_task_groups(task_groups)
   local town = stonehearth.town:get_town(self._entity)
   for i, task_group_name in ipairs(task_groups) do
      town:join_task_group(self._entity, task_group_name)
   end
end

-- Reset this person to their class's default combat stance
-- The stance is set at promotion
function JobComponent:reset_to_default_comabat_stance()
   stonehearth.combat:set_stance(self._entity, self._sv._default_stance)
end

-- Remove this person from a set of task groups
-- @param task_groups - an array of the task groups this person should belong to 
function JobComponent:_remove_from_task_groups(task_groups)
   local town = stonehearth.town:get_town(self._entity)
   for i, task_group_name in ipairs(task_groups) do
      town:leave_task_group(self._entity, task_group_name)
   end
end

function JobComponent:_set_unit_info(name)
   self._entity:add_component('unit_info'):set_description(name)
end

function JobComponent:_equip_outfit(json)
   local equipment_component = self._entity:add_component('stonehearth:equipment')
   if json and json.equipment then
      assert(not self._sv.equipment)
      self._sv.equipment = {}
      
      -- iterate through the equipment in the table, placing one item from each value
      -- on the entity.  they of the entry are irrelevant: they're just for documenation
      for _, items in pairs(json.equipment) do
         local equipment
         if type(items) == 'string' then
            -- create this piece
            equipment = radiant.entities.create_entity(items)
         elseif type(items) == 'table' then
            -- pick an random item from the array
            equipment = radiant.entities.create_entity(items[rng:get_int(1, #items)])
         end
         equipment_component:equip_item(equipment)
         table.insert(self._sv.equipment, equipment)
      end
   end
end

function JobComponent:_remove_outfit()
   if self._sv.equipment then
      local equipment_component = self._entity:add_component('stonehearth:equipment')
      for _, equipment in ipairs(self._sv.equipment) do
         equipment_component:unequip_item(equipment)
         radiant.entities.destroy_entity(equipment)
      end
      self._sv.equipment = nil
   end
end

function JobComponent:_load_job_script(json)
   self._job_script = nil
   if json.script then
      self._job_script = radiant.mods.load_script(json.script)
   end
end

function JobComponent:_call_job_script(fn, ...)
   local script = self._job_script
   if script and script[fn] then
      script[fn](self._entity, ...)
   end
end

return JobComponent
