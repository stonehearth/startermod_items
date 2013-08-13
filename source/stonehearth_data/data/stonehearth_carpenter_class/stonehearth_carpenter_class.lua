--[[
   The carpenter api implements all the functionality that other files will need.
   Promotion_data contains whatever data we need to hook up this specific class.
   In this case, that's a workshop component: {workshop = workshop_component}
]]

local carpenter_class = {}

function carpenter_class.promote(entity, promotion_data)
    radiant.entities.inject_into_entity(entity, '/stonehearth_carpenter_class/class_info/')

    if promotion_data and promotion_data.workshop then
       --Hook the carpenter up to the workbench and vice versa
       local crafter_component = entity:get_component("stonehearth_crafter:crafter")
       local workshop_component = promotion_data.workshop
       crafter_component:set_workshop(workshop_component)
       workshop_component:set_crafter(entity)
    end
end

return carpenter_class