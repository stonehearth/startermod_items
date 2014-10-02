local CarpenterClass = class()

function CarpenterClass:initialize(entity)
   self._sv._entity = entity
   self._sv.last_gained_lv = 0
   self._sv.is_current_class = false
   self._sv.is_max_level = false
end

function CarpenterClass:restore()
end

function CarpenterClass:promote(json)
   self._sv.is_current_class = true
   self._sv.job_name = json.name

   if json.level_data then
      self._sv.level_data = json.level_data
   end

   self._sv._entity:add_component("stonehearth:crafter", json.crafter)

   --Make sure that the build workshop command has a reference to this file
   local command_component = self._sv._entity:get_component('stonehearth:commands')
   if command_component then
      command_component:modify_command('build_workshop', function(command) 
            command.event_data.job_info = '/stonehearth/jobs/carpenter/carpenter_description.json'
         end)
   end

   self.__saved_variables:mark_changed()
end

-- Returns the level the character has in this class
function CarpenterClass:get_job_level()
   return self._sv.last_gained_lv
end

-- Returns whether we're at max level.
-- NOTE: If max level is nto declared, always false
function CarpenterClass:is_max_level()
   return self._sv.is_max_level 
end


function CarpenterClass:demote()
   local workshop_component = self._sv._entity:get_component('stonehearth:crafter'):get_workshop()
   if workshop_component then
      workshop_component:set_crafter(nil)
   end

   self._sv._entity:remove_component("stonehearth:crafter")
   
   --remove workshop orchestrator? Task lists, etc?

   self._sv.is_current_class = false
   self.__saved_variables:mark_changed()

end

return CarpenterClass