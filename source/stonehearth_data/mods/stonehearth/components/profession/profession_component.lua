--[[
   This component stores data about the job that a settler has.
]]
local rng = _radiant.csg.get_default_rng()

local ProfessionComponent = class()

function ProfessionComponent:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()

   if self._sv.profession_uri then
      radiant.events.listen(entity, 'radiant:entity:post_create', function(e)
            self._profession_json = radiant.resources.load_json(self._sv.profession_uri, true)

            if self._profession_json then
               self:_load_profession_script(self._profession_json)
               self:_call_profession_script('restore', self._profession_json)

               --Add self back to the right task groups
               if self._profession_json.task_groups then
                  self:_add_to_task_groups(self._profession_json.task_groups)
               end
            end
            return radiant.events.UNLISTEN
         end)
   end
   self._kill_listener = radiant.events.listen(self._entity, 'stonehearth:kill_event', self, self._on_kill_event)
end

function ProfessionComponent:destroy()
   self._kill_listener:destroy()
   self._kill_listener = nil
end

--When we've been killed, dump our talisman on the ground
function ProfessionComponent:_on_kill_event()
   if self._sv.talisman_uri then
      local location = radiant.entities.get_world_grid_location(self._entity)
      local player_id = radiant.entities.get_player_id(self._entity)
      local output_table = {}
      output_table[self._sv.talisman_uri] = 1
      --TODO: is it possible that this gets dumped onto an inaccessible location?
      radiant.entities.spawn_items(output_table, location, 1, 2, player_id)
   end
end

-- xxx, I think this might be dead code, -Tom
function ProfessionComponent:get_profession_id()
   return self._sv.profession_id
end

function ProfessionComponent:get_profession_uri()
   return self._sv.profession_uri
end

function ProfessionComponent:get_roles()
   return self._profession_json.roles or ''
end

function ProfessionComponent:promote_to(profession_uri)
   self._profession_json = radiant.resources.load_json(profession_uri, true)
   if self._profession_json then
      self:demote()
      self._sv.profession_uri = profession_uri
      self._sv.talisman_uri = self._profession_json.talisman_uri
      
      self._sv.level = 1
      self._sv.current_level_exp = 0
      
      if self._profession_json.levels ~= nil then
         self._sv.total_level_exp = self._profession_json.levels[1].total_level_exp
      else 
         self._sv.total_level_exp = -1
      end
      
      self:_load_profession_script(self._profession_json)
      self:_set_unit_info(self._profession_json)
      self:_equip_outfit(self._profession_json)
      self._sv._default_stance = self._profession_json.default_stance
      self:reset_to_default_comabat_stance()

      self:_call_profession_script('promote', self._profession_json, self._sv.talisman_uri)
      
      if self._profession_json.task_groups then
         self:_add_to_task_groups(self._profession_json.task_groups)
      end
      -- so good!  keep this one, lose the top one.  too much "collusion" between components =)
      radiant.events.trigger(self._entity, 'stonehearth:profession_changed', { entity = self._entity })
      self.__saved_variables:mark_changed()
   end
end

function ProfessionComponent:demote()
   self:_remove_outfit()
   self:_call_profession_script('demote')

   self._sv._default_stance = 'passive'
   self:reset_to_default_comabat_stance()

   if self._profession_json and self._profession_json.task_groups then
      self:_remove_from_task_groups(self._profession_json.task_groups)
   end

   self._sv.profession_id = nil
   self.__saved_variables:mark_changed()
end

function ProfessionComponent:add_exp(value)
   self._sv.current_level_exp = self._sv.current_level_exp + value

   while self._sv.current_level_exp > self._sv.total_level_exp do
      self._sv.level = self._sv.level + 1
      self._sv.current_level_exp = self._sv.current_level_exp - self._sv.total_level_exp

      self:_call_profession_script('level_up')

      -- we've leveled up, increment the exp to next level
      self._sv.total_level_exp = self._profession_json.levels[self._sv.level].total_level_exp
   end

   self.__saved_variables:mark_changed()   
end

function ProfessionComponent:level_up()

end

--- Adds this person to a set of task groups
--  @param task_groups - an array of the task groups this person should belong to
function ProfessionComponent:_add_to_task_groups(task_groups)
   local town = stonehearth.town:get_town(self._entity)
   for i, task_group_name in ipairs(task_groups) do
      town:join_task_group(self._entity, task_group_name)
   end
end

-- Reset this person to their class's default combat stance
-- The stance is set at promotion
function ProfessionComponent:reset_to_default_comabat_stance()
   stonehearth.combat:set_stance(self._entity, self._sv._default_stance)
end

-- Remove this person from a set of task groups
-- @param task_groups - an array of the task groups this person should belong to 
function ProfessionComponent:_remove_from_task_groups(task_groups)
   local town = stonehearth.town:get_town(self._entity)
   for i, task_group_name in ipairs(task_groups) do
      town:leave_task_group(self._entity, task_group_name)
   end
end

function ProfessionComponent:_set_unit_info(json)
   if json and json.name then
      self._entity:add_component('unit_info'):set_description(json.name)
   end
end

function ProfessionComponent:_equip_outfit(json)
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

function ProfessionComponent:_remove_outfit()
   if self._sv.equipment then
      local equipment_component = self._entity:add_component('stonehearth:equipment')
      for _, equipment in ipairs(self._sv.equipment) do
         equipment_component:unequip_item(equipment)
         radiant.entities.destroy_entity(equipment)
      end
      self._sv.equipment = nil
   end
end

function ProfessionComponent:_load_profession_script(json)
   self._profession_script = nil
   if json.script then
      self._profession_script = radiant.mods.load_script(json.script)
   end
end

function ProfessionComponent:_call_profession_script(fn, ...)
   local script = self._profession_script
   if script and script[fn] then
      script[fn](self._entity, ...)
   end
end

return ProfessionComponent
