local weaver_class = {}

function weaver_class.promote(entity, workshop_component)

   --Add the necessary components
   local profession_description = radiant.resources.load_json('stonehearth:weaver:profession_description')
   entity:add_component('stonehearth:profession', profession_description.profession)
   entity:add_component("stonehearth:crafter", profession_description.crafter)

   --Slap a new outfit on the crafter
   local equipment = entity:add_component('stonehearth:equipment')
   equipment:equip_item('stonehearth:weaver:outfit')

   --Make sure that the build workshop command has a reference to this file
   local command_component = entity:get_component('stonehearth:commands')
   if command_component then
      command_component:modify_command('build_workshop', function(command) 
            command.event_data.profession_info = '/stonehearth/professions/weaver/weaver_description.json'
         end)
   end
end

function weaver_class.demote(entity)
   local equipment = entity:add_component('stonehearth:equipment')
   local outfit = equipment:unequip_item('stonehearth:worker_outfit')
   
   if outfit then
      radiant.entities.destroy_entity(outfit)
   end

  --move saw back to the bench
  --local crafter_component = entity:get_component("stonehearth:crafter")
  --local bench_component = crafter_component:get_workshop()
end

return weaver_class
