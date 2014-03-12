local priorities = require('constants').priorities.worker_task
local farming_service = stonehearth.farming

local ResourceCallHandler = class()

--- Remote entry point for requesting that a tree get harvested
-- @param tree The entity which you would like chopped down
-- @return true on success, false on failure
function ResourceCallHandler:harvest_tree(session, response, tree)
   local town = stonehearth.town:get_town(session.faction)
   return town:harvest_resource_node(tree)
end

function ResourceCallHandler:harvest_plant(session, response, plant)
   local town = stonehearth.town:get_town(session.faction)
   return town:harvest_renewable_resource_node(plant)
end

function ResourceCallHandler:shear_sheep(session, response, sheep)
   asset(false, 'shear_sheep was broken, so i deleted it =)  -tony')
end

--TODO: Send an array of soil_plots and the type of the crop for batch planting
function ResourceCallHandler:plant_crop(session, response, soil_plot, crop_type)
   --TODO: remove this when we actually get the correct data from the UI
   local soil_plots = {soil_plot}
   if not crop_type then
      crop_type = 'stonehearth:corn_seedling'
   end

   return farming_service:plant_crop(session.faction, soil_plots, crop_type)
end

--TODO: Send an array of soil_plots and the type of the crop for batch planting
function ResourceCallHandler:break_down_object(session, response, target_entity, target_actors)
   local town = stonehearth.town:get_town(session.faction)

   --TODO: remove this when we actually get the correct data from the UI
   --local soil_plots = {soil_plot}
   --if not crop_type then
   --   crop_type = 'stonehearth:corn_seedling'
   --end

   --return town:plant_crop(soil_plots, crop_type)
end

return ResourceCallHandler
