local CarpenterClass = class()

function CarpenterClass:initialize(entity)
   self._sv._entity = entity
   self._sv.last_gained_lv = 0
end

function CarpenterClass:restore()
end

function CarpenterClass:promote(json)
   self._sv._entity:add_component("stonehearth:crafter", json.crafter)

   --Make sure that the build workshop command has a reference to this file
   local command_component = self._sv._entity:get_component('stonehearth:commands')
   if command_component then
      command_component:modify_command('build_workshop', function(command) 
            command.event_data.job_info = '/stonehearth/jobs/carpenter/carpenter_description.json'
         end)
   end
end

function CarpenterClass:demote()
   --TODO: Fix! Currently, there is no entity:remove_component!
   self._sv._entity:remove_component("stonehearth:crafter")
end

return CarpenterClass