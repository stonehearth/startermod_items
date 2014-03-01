local carpenter_class = {}

function carpenter_class.promote(entity)

   --Add the necessary components
   local profession_description = radiant.resources.load_json('stonehearth:carpenter:profession_description')
   entity:add_component('stonehearth:profession', profession_description.profession)
   entity:add_component("stonehearth:crafter", profession_description.crafter)

   --Slap a new outfit on the crafter
   local equipment = entity:add_component('stonehearth:equipment')
   equipment:equip_item('stonehearth:carpenter:outfit')

   --Make sure that the build workshop command has a reference to this file
   local command_component = entity:get_component('stonehearth:commands')
   if command_component then
      command_component:modify_command('build_workshop', function(command) 
            command.event_data.profession_info = '/stonehearth/entities/professions/carpenter/profession_description.json'
         end)
   end
end

function carpenter_class.demote(entity)
   local equipment = entity:add_component('stonehearth:equipment')
   local outfit = equipment:unequip_item('stonehearth:worker_outfit')
   
   if outfit then
      radiant.entities.destroy_entity(outfit)
   end
end

return carpenter_class
