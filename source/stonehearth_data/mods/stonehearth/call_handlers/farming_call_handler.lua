local Color4 = _radiant.csg.Color4

local farming_service = stonehearth.farming

local FarmingCallHandler = class()

-- runs on the client!!
function FarmingCallHandler:choose_new_field_location(session, response)
   stonehearth.selection:select_xz_region()
      :restrict_to_standable_terrain()
      :use_designation_marquee(Color4(55, 187, 56, 255))
      :set_cursor('stonehearth:cursors:designate_zone')
      :done(function(selector, box)
            local size = {
               x = box.max.x - box.min.x,
               y = box.max.z - box.min.z,
            }
            _radiant.call('stonehearth:create_new_field', box.min, size)
                     :done(function(r)
                           response:resolve({ field = r.field })
                        end)
                     :always(function()
                           selector:destroy()
                        end)         
         end)
      :fail(function(selector)
            selector:destroy()
            response:reject('no region')
         end)
      :go()
end

-- runs on the server!
function FarmingCallHandler:create_new_field(session, response, location, size)
   local entity = stonehearth.farming:create_new_field(session, location, size)
   return { field = entity }
end

--TODO: Send an array of soil_plots and the type of the crop for batch planting
function FarmingCallHandler:plant_crop(session, response, soil_plot, crop_type, player_speficied, auto_plant, auto_harvest)
   --TODO: remove this when we actually get the correct data from the UI
   local soil_plots = {soil_plot}
   if not crop_type then
      crop_type = 'stonehearth:turnip_crop'
   end

   return farming_service:plant_crop(session.player_id, soil_plots, crop_type, player_speficied, auto_plant, auto_harvest, true)
end

--TODO: Send an array of soil_plots and the type of the crop for batch planting
--Give a set of plots to clear, harvest the plants inside
function FarmingCallHandler:raze_crop(session, response, soil_plot)
   --TODO: remove this when we actually get the correct data from the UI
   local soil_plots = {soil_plot}
   for i, plot in ipairs(soil_plots) do 
      local plot_component = plot:get_component('stonehearth:dirt_plot') 
   end

   return farming_service:harvest_crops(session, soil_plots)
end

--- Returns the crops available for planting to this player
function FarmingCallHandler:get_all_crops(session)
   return {all_crops = farming_service:get_all_crop_types(session)}
end

return FarmingCallHandler
