local Point3 = _radiant.csg.Point3
local CreateStockpile = class()

-- runs on the client!!
function CreateStockpile:choose_stockpile_location(session, response)
   -- create a new "cursor entity".  this is the entity that will move around the
   -- screen to preview where the workbench will go.  these entities are called
   -- "authoring entities", because they exist only on the client side to help
   -- in the authoring of new content.
   local cursor_entity = radiant.entities.create_entity()
   local mob = cursor_entity:add_component('mob')
   mob:set_interpolate_movement(false)
   
   -- add a render object so the cursor entity gets rendered.
   local cursor_render_entity = _client:create_render_entity(1, cursor_entity)
   local node = h3dRadiantCreateStockpileNode(cursor_render_entity:get_node(), 'stockpile designation')

   local cleanup = function(data)
      data = data and {}
      _client:destroy_authoring_entity(cursor_entity:get_id())
      response:resolve(data)
   end

   _client:select_xz_region()
      :progress(function (p0, p1)
            mob:set_location_grid_aligned(p0)
            h3dRadiantResizeStockpileNode(node, p1.x - p0.x + 1, p1.z - p0.z + 1);
         end)
      :done(function (p0, p1)
            local size = {
               p1.x - p0.x + 1,
               p1.z - p0.z + 1,
            }
            _radiant.call_obj('stonehearth_inventory', 'create_stockpile', p0, size)
                     :always(function(data)
                           cleanup(data)
                        end)
         end)
      :fail(function()
            cleanup()
         end)
end

-- runs on the server!
function CreateStockpile:create_stockpile(session, response, location, size)
   local inventory = require('api').get_inventory(session.faction)
   inventory:create_stockpile(location, size)
   return true
end

return CreateStockpile
