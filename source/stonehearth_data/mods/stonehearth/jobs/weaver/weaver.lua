local WeaverClass = class()

function WeaverClass:initialize(entity)
   self._sv._entity = entity
   self._sv.last_gained_lv = 0
   self._sv.is_current_class = false
   self._sv.is_max_level = false
end

function WeaverClass:promote(json)
   self._sv.is_current_class = true
   self._sv.job_name = json.name

   self._sv._entity:add_component("stonehearth:crafter", json.crafter)

   --Make sure that the build workshop command has a reference to this file
   local command_component = self._sv._entity:get_component('stonehearth:commands')
   if command_component then
      command_component:modify_command('build_workshop', function(command) 
            command.event_data.job_info = '/stonehearth/jobs/weaver/weaver_description.json'
         end)
   end

   self.__saved_variables:mark_changed()
end

-- Returns the level the character has in this class
function WeaverClass:get_job_level()
   return self._sv.last_gained_lv
end

-- Returns whether we're at max level.
-- NOTE: If max level is nto declared, always false
function WeaverClass:is_max_level()
   return self._sv.is_max_level 
end

function WeaverClass:demote()
   --TODO: not implemented
   entity:remove_component("stonehearth:crafter")

   self._sv.is_current_class = false
   self.__saved_variables:mark_changed()
end

return WeaverClass
