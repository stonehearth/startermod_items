local WeaverClass = class()

function WeaverClass:initialize(entity)
   self._sv._entity = entity
   self._sv.last_gained_lv = 0
end

function WeaverClass:promote(json)
   self._sv._entity:add_component("stonehearth:crafter", json.crafter)

   --Make sure that the build workshop command has a reference to this file
   local command_component = self._sv._entity:get_component('stonehearth:commands')
   if command_component then
      command_component:modify_command('build_workshop', function(command) 
            command.event_data.profession_info = '/stonehearth/professions/weaver/weaver_description.json'
         end)
   end
end

function WeaverClass:demote()
   --TODO: not implemented
   entity:remove_component("stonehearth:crafter")
end

return WeaverClass
