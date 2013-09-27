--[[
   The carpenter api implements all the functionality that other files will need.
   Promotion_data contains whatever data we need to hook up this specific class.
   In this case, that's a workshop component: {workshop = workshop_component}
]]

local carpenter_class = {}

function carpenter_class.promote(entity, promotion_data)
   assert(promotion_data.workshop)

   --Add the necessary components
   local job_description = radiant.resources.load_json('stonehearth.carpenter.job_description')
   local job_info_component = entity:add_component('stonehearth:job_info')
   job_info_component:set_info(job_description.job_info)

   local crafter_component = entity:add_component("stonehearth:crafter")
   crafter_component:set_info(job_description.crafter)

   local workshop_component = promotion_data.workshop
   crafter_component:set_workshop(workshop_component)
   workshop_component:set_crafter(entity)


   --Slap a new outfit on the crafter
   local equipment = entity:add_component('stonehearth:equipment')
   equipment:equip_item('stonehearth.carpenter_outfit')


end

function carpenter_class.demote(entity)
   local equipment = entity:add_component('stonehearth:equipment')
   local outfit = equipment:unequip_item('stonehearth.worker_outfit')
   
   if outfit then
      radiant.entities.destroy_entity(outfit)
   end

  --move saw back to the bench
  --local crafter_component = entity:get_component("stonehearth:crafter")
  --local bench_component = crafter_component:get_workshop()
end

return carpenter_class
