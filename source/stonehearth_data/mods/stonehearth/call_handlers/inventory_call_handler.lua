local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Color4 = _radiant.csg.Color4
local Rect2 = _radiant.csg.Rect2
local Point2 = _radiant.csg.Point2
local InventoryCallHandler = class()

-- runs on the client!!
function InventoryCallHandler:choose_stockpile_location(session, response)

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
                                     Point2(box.max.x - box.min.x + 1, box.max.z - box.min.z + 1)))
            end)
            mob:set_location_grid_aligned(box.min)
            if node then
               h3dRemoveNode(node)
            end
            node = _radiant.client.create_designation_node(parent_node, self._region:get(), Color4(0, 153, 255, 255), Color4(0, 153, 255, 255));
         end)
      :done(function (box)
            local size = {
               x = box.max.x - box.min.x + 1,
               y = box.max.z - box.min.z + 1,
            }
            _radiant.call('stonehearth:create_stockpile', box.min, size)
                     :done(function(r)
                           response:resolve({
                                 stockpile = r.stockpile
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
function InventoryCallHandler:create_stockpile(session, response, location, size)
   local inventory = stonehearth.inventory:get_inventory(session.player_id)
   local stockpile = inventory:create_stockpile(location, size)
   return { stockpile = stockpile }
end

return InventoryCallHandler
