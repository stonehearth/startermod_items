local carpenter_class = {}

function carpenter_class.promote(entity, json)
   entity:add_component("stonehearth:crafter", json.crafter)

   --Make sure that the build workshop command has a reference to this file
   local command_component = entity:get_component('stonehearth:commands')
   if command_component then
      command_component:modify_command('build_workshop', function(command) 
            command.event_data.profession_info = '/stonehearth/professions/carpenter/carpenter_description.json'
         end)
   end

   local weapon = radiant.entities.create_entity('stonehearth:carpenter:saw')
   radiant.entities.equip_item(entity, weapon, 'melee_weapon')

   -- HACK: remove the talisman glow effect from the weapon
   radiant.entities.remove_effects(weapon)

   stonehearth.combat:set_stance(entity, 'passive')
end

function carpenter_class.demote(entity)
   --TODO: Fix! Currently, there is no entity:remove_component!
   entity:remove_component("stonehearth:crafter")
end

return carpenter_class
