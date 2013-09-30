local Point3 = _radiant.csg.Point3
local InventoryCallHandler = class()

-- runs on the client!!
function InventoryCallHandler:choose_stockpile_location(session, response)

   -- create a new "cursor entity".  this is the entity that will move around the
   -- screen to preview where the workbench will go.  these entities are called
   -- "authoring entities", because they exist only on the client side to help
   -- in the authoring of new content.
   local cursor_entity = radiant.entities.create_entity()
   local mob = cursor_entity:add_component('mob')
   mob:set_interpolate_movement(false)
   
   -- add a render object so the cursor entity gets rendered.
   local cursor_render_entity = _radiant.client.create_render_entity(1, cursor_entity)
   local node = h3dRadiantCreateStockpileNode(cursor_render_entity:get_node(), 'stockpile designation')

   -- change the actual game cursor
   local stockpile_cursor = _radiant.client.set_cursor('stonehearth.cursors.create_stockpile')

   local cleanup = function()
      stockpile_cursor:destroy()
      _radiant.client.destroy_authoring_entity(cursor_entity:get_id())
   end

   _radiant.client.select_xz_region()
      :progress(function (box)
            mob:set_location_grid_aligned(box.min)
            h3dRadiantResizeStockpileNode(node, box.max.x - box.min.x + 1, box.max.z - box.min.z + 1);
         end)
      :done(function (box)
            local size = {
               box.max.x - box.min.x + 1,
               box.max.z - box.min.z + 1,
            }
            _radiant.call('stonehearth.create_stockpile', box.min, size)
                     :done(function(r)
                           response:resolve(r)
                        end)
                     :always(function()
                           cleanup()
                        end)
         end)
      :fail(function()
            cleanup()
         end)
end

-- runs on the server!
function InventoryCallHandler:create_stockpile(session, response, location, size)
   local inventory = radiant.mods.load('stonehearth').inventory:get_inventory(session.faction)
   inventory:create_stockpile(location, size)
   return true
end

return InventoryCallHandler
