--[[
   The carpenter api implements all the functionality that other files will need.
   Promotion_data contains whatever data we need to hook up this specific class.
   In this case, that's a workshop component: {workshop = workshop_component}
]]

local carpenter_class = {}

function carpenter_class.promote(entity, promotion_data)
   -- xxx: bad!  gotta get rid of inject (tony)
    radiant.entities.xxx_inject_into_entity(entity, 'stonehearth_carpenter_class.class_info')

    if promotion_data and promotion_data.workshop then
       --Hook the carpenter up to the workbench and vice versa
       local crafter_component = entity:get_component("stonehearth:crafter")
       local workshop_component = promotion_data.workshop
       crafter_component:set_workshop(workshop_component)
       workshop_component:set_crafter(entity)
    end

   -- xxx: this is strictly temporary.  the code will be factored into stonehearth_classes soon.
   local outfit = radiant.entities.create_entity('stonehearth_carpenter_class.carpenter_outfit')
   local render_info = entity:add_component('render_info')
   render_info:attach_entity(outfit)
   -- end xxx:
end

function carpenter_class.demote(entity)
  --move saw back to the bench
  --local crafter_component = entity:get_component("stonehearth:crafter")
  --local bench_component = crafter_component:get_workshop()



end

return carpenter_class
