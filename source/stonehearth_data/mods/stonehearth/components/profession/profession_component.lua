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

function ProfessionComponent:promote_to(profession_uri, talisman_uri)
   self._profession_json = radiant.resources.load_json(profession_uri, true)
   if self._profession_json then
      self:demote()
      self._sv.profession_uri = profession_uri
      self:_load_profession_script(self._profession_json)
      self:_set_unit_info(self._profession_json)
      self:_equip_outfit(self._profession_json)
      self:_call_profession_script('promote', self._profession_json, talisman_uri)

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

   if self._profession_json and self._profession_json.task_groups then
      self:_remove_from_task_groups(self._profession_json.task_groups)
   end

   self._sv.profession_id = nil
   self.__saved_variables:mark_changed()
end

--- Adds this person to a set of task groups
--  @param task_groups - an array of the task groups this person should belong to
function ProfessionComponent:_add_to_task_groups(task_groups)
   local town = stonehearth.town:get_town(self._entity)
   for i, task_group_name in ipairs(task_groups) do
      town:join_task_group(self._entity, task_group_name)
   end
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
