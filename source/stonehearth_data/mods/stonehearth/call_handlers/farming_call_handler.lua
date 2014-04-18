local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Color4 = _radiant.csg.Color4
local Rect2 = _radiant.csg.Rect2
local Point2 = _radiant.csg.Point2

local farming_service = stonehearth.farming

local FarmingCallHandler = class()

-- runs on the client!!
function FarmingCallHandler:choose_new_field_location(session, response)

   self._region = _radiant.client.alloc_region2()
   
   -- create a new "cursor entity".  this is the entity that will move around the
   -- screen to preview where the workbench will go.  these entities are called
   -- "authoring entities", because they exist only on the client side to help
   -- in the authoring of new content.
   local cursor_entity = radiant.entities.create_entity()
   local mob = cursor_entity:add_component('mob')
   
   -- add a render object so the cursor entity gets rendered.
   local cursor_render_entity = _radiant.client.create_render_entity(1, cursor_entity)
   local parent_node = cursor_render_entity:get_node()
   local node

   -- change the actual game cursor
   local xz_selector
   local stockpile_cursor = _radiant.client.set_cursor('stonehearth:cursors:create_stockpile')

   local cleanup = function()
      if node then
         h3dRemoveNode(node)
      end
      xz_selector:destroy()
      stockpile_cursor:destroy()
      _radiant.client.destroy_authoring_entity(cursor_entity:get_id())
   end


   xz_selector = _radiant.client.select_xz_region()
      :progress(function (box)
            self._region:modify(function(cursor)
               cursor:clear()
               cursor:add_cube(Rect2(Point2(0, 0), 
                                     Point2(box.max.x - box.min.x, box.max.z - box.min.z)))
            end)
            mob:set_location_grid_aligned(box.min)
            if node then
               h3dRemoveNode(node)
            end
            node = _radiant.client.create_designation_node(parent_node, self._region:get(), Color4(122, 40, 0, 255), Color4(122, 40, 0, 255));
         end)
      :done(function (box)
            local size = {
               box.max.x - box.min.x,
               box.max.z - box.min.z,
            }
            _radiant.call('stonehearth:create_new_field', box.min, size)
                     :done(function(r)
                           response:resolve({
                                 field = r.field
                              })
                        end)
                     :always(function()
                           cleanup()
                        end)
         end)
      :fail(function()
            cleanup()
            response:resolve(false)
         end)
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
