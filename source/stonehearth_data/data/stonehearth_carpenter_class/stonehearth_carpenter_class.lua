--[[
   The carpenter api implements all the functionality that other files will need.
   Profession_info contains whatever data we need to hook up this specific class
]]

local carpenter_class = {}

function carpenter_class.promote(entity, profession_info)
    radiant.entities.inject_into_entity(entity, '/stonehearth_carpenter_class/class_info/')

    if profession_info then
       --Hook the carpenter up to the workbench and vice versa
       local crafter_component = entity:get_component("stonehearth_crafter:crafter")
       local workshop_component = profession_info:get_workshop()
       crafter_component:set_workshop(workshop_component)
       workshop_component:set_crafter(entity)
    end
end

return carpenter_class