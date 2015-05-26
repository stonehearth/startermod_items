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
      self._sv.curr_job_controller = self._sv.job_controllers[self._sv.job_uri]
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

   --destroy all the controllers
   for uri, controller in pairs(self._sv.job_controllers) do
      controller:destroy()
   end
   self._sv.curr_job_controller = nil

   --Remove from task groups (may be redundant if we are killed, which first calls demote, then this)
   if self._job_json and self._job_json.task_groups then
      self:_remove_from_task_groups(self._job_json.task_groups)
   end

end

--When we've been killed, dump our talisman on the ground
--Eventually, when our entity is destroyed, the destroy fn above will run also.
function JobComponent:_on_kill_event()
   if not stonehearth.player:is_npc(self._entity) then
      self:demote()
   end
end

function JobComponent:_call_job(method_name, ...)
   local controller = self._sv.curr_job_controller
   local fn = controller[method_name]
   if fn then
      return fn(controller, ...)
   end
   return nil
end


-- Drops the talisman near the location of the entity, returns the talisman entity
-- If the class has something to say about the talisman before it goes, do that first
function JobComponent:_drop_talisman()
   if not stonehearth.player:is_npc(self._entity) then
      if self._sv.talisman_uri then
         local location = radiant.entities.get_world_grid_location(self._entity)
         local player_id = radiant.entities.get_player_id(self._entity)
         local output_table = {}
         output_table[self._sv.talisman_uri] = 1
         --TODO: is it possible that this gets dumped onto an inaccessible location?
         local items = radiant.entities.spawn_items(output_table, location, 1, 2, { owner = player_id })

         for id, obj in pairs(items) do
            if obj:get_component('stonehearth:promotion_talisman') then
               self:_call_job('associate_entities_to_talisman', obj)
               return obj
            end
         end
      end
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
      local level = self:_call_job('get_job_level') or 0
      new_title = self:_compose_default_job_name(job_json, level, job_json.name)
   end
   return new_title
end

-- Look up the level of the character in this job and
-- see if there is a specific title for that level
-- if so, return it, if not, return nil. 
function JobComponent:_get_title_for_level(job_json)
   local curr_level = self:_call_job('get_job_level') or 0

   if job_json.level_data then
      local lv_data = job_json.level_data[tostring(curr_level)]
      if not lv_data then return nil end

      if lv_data and lv_data.title then
         return lv_data.title
      end
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

-- Promotes a citizen to a new job. Calls demote on existing job. 
-- Called in the moment that the animation converts the citizen to their new job. 
-- @job_uri - uri of the job we're promoting to
-- @talisman_entity - specific talisman associated with this job, optional
function JobComponent:promote_to(job_uri, options)
   local is_npc = stonehearth.player:is_npc(self._entity)
   local talisman_entity = options and options.talisman

   self._job_json = radiant.resources.load_json(job_uri, true)
   if self._job_json then
      self:demote()
      self._sv.job_uri = job_uri
      self._sv.talisman_uri = self._job_json.talisman_uri
      self._sv.class_icon = self._job_json.icon

      --Strangely, doesn't exist yet when this is called in init, creates duplicate component!
      if not self._sv.attributes_component then
         self._sv.attributes_component = self._entity:get_component('stonehearth:attributes')
         self._sv.total_level = self._sv.attributes_component:get_attribute('total_level')
      end

      --Whenever you get a new job, dump all the xp that you've accured so far to your next level
      self._sv.current_level_exp = 0
      self._sv.xp_to_next_lv = self:_calculate_xp_to_next_lv()

      -- equip your abilities item
      self:_equip_abilities(self._job_json)

      -- equip your equipment, unless you're an npc, in which case the game is responsible for manually
      -- adding the proper equipment
      if not is_npc then
         self:_equip_equipment(self._job_json)
      end

      self._sv._default_stance = self._job_json.default_stance
      self:reset_to_default_comabat_stance()

      --Create the job controller, if we don't yet have one
      if not self._sv.job_controllers[self._sv.job_uri] then
         --create the controller
         self._sv.job_controllers[self._sv.job_uri] = 
            radiant.create_controller(self._job_json.controller, self._entity)
      end
      local player_id = radiant.entities.get_player_id(self._entity)
      self._sv.curr_job_info = stonehearth.job:get_job_info(player_id, job_uri)
      self._sv.curr_job_controller = self._sv.job_controllers[self._sv.job_uri]
      self:_call_job('promote', self._job_json, {talisman = talisman_entity})
      self._sv.curr_job_name = self:_get_current_job_title(self._job_json)
      self:_set_unit_info(self._sv.curr_job_name)

      --Add all existing perks, if any
      self:_apply_existing_perks()
      
      -- remove any acquired equipment from our prior job that we can no longer use
      self:_remove_unequippable_items()

      --Add self to task groups
      if self._job_json.task_groups then
         self:_add_to_task_groups(self._job_json.task_groups)
      end

      --Log in journal, if possible
      local activity_name = self._job_json.promotion_activity_name
      if activity_name then
         radiant.events.trigger_async(stonehearth.personality, 'stonehearth:journal_event', 
                             {entity = self._entity, description = activity_name})
      end

      -- so good!  keep this one, lose the top one.  too much "collusion" between components =)
      radiant.events.trigger(self._entity, 'stonehearth:job_changed', { entity = self._entity })
      self.__saved_variables:mark_changed()
   end
end

--- Given the ID of a perk, find out of the current class has unlocked that perk. 
function JobComponent:curr_job_has_perk(id)
   return self:_call_job('has_perk', id) or false
end

--If we've been this class before, re-apply any perks we've gained
function JobComponent:_apply_existing_perks()
   local last_gained_lv = self:_call_job('get_job_level') or 0
   if last_gained_lv == 0 then 
      return
   end

   for i=1, last_gained_lv do
      self:_apply_perk_for_level(i)
   end 
end

-- Given a level, apply all perks relevant to that job
-- Uses the current job controller's functions to do this. 
function JobComponent:_apply_perk_for_level(target_level)
   local level_data = self:_call_job('get_level_data') or {}
   local job_updates_for_level = level_data[tostring(target_level)]
   local perk_descriptions = {}
   if job_updates_for_level then
      for i, perk_data in ipairs(job_updates_for_level.perks) do
         self:_call_job('unlock_perk', perk_data.id)
         if perk_data.type then
            self:_call_job(perk_data.type, perk_data)
         end

         --Collect text about the perk
         local perk_info = {
            name = perk_data.name,
            description = perk_data.description, 
            icon = perk_data.icon
         }
         table.insert(perk_descriptions, perk_info)
      end
   end
   return perk_descriptions
end

--Itereate through each perk we have and call its remove function
function JobComponent:_remove_all_perks()
   if not self._sv.curr_job_controller then
      return
   end
   local last_gained_lv = self:_call_job('get_job_level') or 0
   if last_gained_lv == 0 then
      return
   end

   local level_data = self:_call_job('get_level_data') or {}
   for i=1, last_gained_lv do
      local job_updates_for_level = level_data[tostring(i)]
      if job_updates_for_level then
         for i, perk_data in ipairs(job_updates_for_level.perks) do
            if perk_data.demote_fn then
               self:_call_job(perk_data.demote_fn, perk_data)
            end
         end
      end
   end 
end

--Call when we no longer want the job we have
function JobComponent:demote()
   self:_remove_profession_equipment()
   self:_remove_all_perks()

   --If we have a talisman to drop, drop it. 
   local talisman = self:_drop_talisman()

   if self._sv.curr_job_controller then
      self:_call_job('demote')
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
   if not self._sv.xp_equation then
      -- no xp_equation means this race cannot level up.  ignore.
      return
   end

   local max_level = self:_call_job('is_max_level')
   if max_level == true or max_level == nil then
      return
   end

   self._sv.current_level_exp = self._sv.current_level_exp + value

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
   self._sv.total_level = curr_level + 1
   self._sv.attributes_component:set_attribute('total_level', self._sv.total_level)

   --Add all the universal level dependent buffs/bonuses, etc

   self:_call_job('level_up')
   self._sv.curr_job_name = self:_get_current_job_title(self._job_json)
   self:_set_unit_info(self._sv.curr_job_name)   
   local new_level = self:_call_job('get_job_level') or 0
   local job_name = self._job_json.name
   local class_perk_descriptions = self:_apply_perk_for_level(new_level)
   local has_class_perks = false
   if #class_perk_descriptions > 0 then
      has_class_perks = true
   end

   local player_id = radiant.entities.get_player_id(self._entity)
   local name = radiant.entities.get_display_name(self._entity)
   local title = self._sv._default_level_announcement
   title = string.gsub(title, '__name__', name)
   title = string.gsub(title, '__job_name__', job_name)
   title = string.gsub(title, '__level_number__', new_level)

   local has_race_perks = false
   local race_perk_descriptions = self:_add_race_perks()
   if #race_perk_descriptions > 0 then
      has_race_perks = true
   end
   
   --post the bulletin
   local level_bulletin = stonehearth.bulletin_board:post_bulletin(player_id)
      :set_ui_view('StonehearthLevelUpBulletinDialog')
      :set_callback_instance(self)
      :set_type('level_up')
      :set_data({
         title = title, 
         char_name = name,
         zoom_to_entity = self._entity, 
         has_class_perks = has_class_perks, 
         class_perks = class_perk_descriptions,
         has_race_perks = has_race_perks, 
         race_perks = race_perk_descriptions
      })
      :set_active_duration('6h')

   --Trigger an event so people can extend the class system
   radiant.events.trigger_async(self._entity, 'stonehearth:level_up', {
      level = new_level, 
      job_uri = self._sv.job_uri, 
      job_name = self._sv.curr_job_name })

   radiant.effects.run_effect(self._entity, '/stonehearth/data/effects/level_up')
   
   --TODO: localize, decide how we want to announce this
   --TODO: update UI/char sheet, etc
   --TODO: have this affect happiness!
   --local name = radiant.entities.get_display_name(self._entity)
   --stonehearth.events:add_entry(name .. ' has leveled up in ' .. self._sv.curr_job_name .. '!')
   self._sv.xp_to_next_lv = self:_calculate_xp_to_next_lv()
   self.__saved_variables:mark_changed()   
end

--TOTAL HACK!!!!
--TODO: how to add this gracefully? Factor out to a race controller?
--TODO: we don't handle race very well yet, this should only be true IF we are human
function JobComponent:_add_race_perks()
   local race_perks = {}
   local human_level_perk = {
      name = 'HP +10', 
      icon = '/stonehearth/data/images/race/human_HP_on_level.png'
   }
   table.insert(race_perks, human_level_perk)
   return race_perks
end

function JobComponent:get_xp_to_next_lv()
   return self._sv.xp_to_next_lv
end

function JobComponent:get_curr_job_controller()
   return self._sv.curr_job_controller
end

-- Calculate the exp requried to get to the level after this one. 
function JobComponent:_calculate_xp_to_next_lv()
   if not self._sv.xp_equation then
      return nil
   end
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
      town:leave_task_group(self._entity:get_id(), task_group_name)
   end
end

--Set the description of the person to something appropriate for their job
--If they are an NPC, don't change this!
function JobComponent:_set_unit_info(name)
   if not stonehearth.player:is_npc(self._entity) then
      self._entity:add_component('unit_info'):set_description(name)
   end
end

function JobComponent:_equip_abilities(json)
   assert(json and json.abilities)
   assert(not self._sv.profession_equipment)
      
   
   local equipment_component = self._entity:add_component('stonehearth:equipment')
   local item = radiant.entities.create_entity(json.abilities)
   equipment_component:equip_item(item)
   
   self._sv.profession_equipment = {}
   table.insert(self._sv.profession_equipment, item)
end

function JobComponent:_equip_equipment(json)
   local equipment_component = self._entity:add_component('stonehearth:equipment')
   if json and json.equipment then
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
         table.insert(self._sv.profession_equipment, equipment)
      end
   end
end

-- Drop all the equipment and the talisman, if relevant
function JobComponent:_remove_profession_equipment()
   if self._sv.profession_equipment then
      -- make sure we only take away what we gave the entity.  otherwise, we may end
      -- up nuking abilities which were given by other parts of the the code (for example,
      -- party abilities)
      for i, item in ipairs(self._sv.profession_equipment) do
         self:_remove_item(item)
      end
      self._sv.profession_equipment = nil
   end

   --TODO: what to do about backpack? Should this be called or triggered via an event?
   --or should this be handled by the demote operation on the specific job controller?
end

function JobComponent:_remove_unequippable_items()
   local profession_equipment = self:_entity_array_to_map(self._sv.profession_equipment)
   local equipment_component = self._entity:add_component('stonehearth:equipment')
   local equipped_items = equipment_component:get_all_items()

   for _, item in pairs(equipped_items) do
      -- Profession equipment is by definition equippable. We perform this check so that we
      -- don't require all basic equipment to be tagged with the proper roles.
      if not profession_equipment[item:get_id()] then
         local equipment_piece_component = item:add_component('stonehearth:equipment_piece')
         if not equipment_piece_component:is_equippable_by(self._entity) then
            self:_remove_item(item)
         end
      end
   end
end

function JobComponent:_remove_item(item)
   local equipment_component = self._entity:add_component('stonehearth:equipment')
   equipment_component:unequip_item(item)

   local equipment_piece_component = item:get_component('stonehearth:equipment_piece')
   if equipment_piece_component and equipment_piece_component:get_should_drop() then
      self:_drop_item(item)
   else
      -- this will make sure we don't leak any job ability and other non-tangible
      -- equipment pieces that we created during the promote process.
      radiant.entities.destroy_entity(item)
   end
end

function JobComponent:_drop_item(item)
   local location = radiant.entities.get_world_grid_location(self._entity)
   local placement_point = radiant.terrain.find_placement_point(location, 1, 4)
   radiant.terrain.place_entity(item, placement_point)
end

function JobComponent:_entity_array_to_map(array)
   local map = {}

   for _, entity in pairs(array) do
      map[entity:get_id()] = entity
   end

   return map
end

--[[
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
]]

return JobComponent
