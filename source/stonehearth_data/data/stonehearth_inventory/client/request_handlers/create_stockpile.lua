local Point3 = _radiant.csg.Point3
local CreateStockpile = class()

-- client side object to add a new bench to the world.  this method is invoked
-- by POSTing to the route for this file in the manifest.
function CreateStockpile:handle_request(query, postdata, response)
   -- create a new "cursor entity".  this is the entity that will move around the
   -- screen to preview where the workbench will go.  these entities are called
   -- "authoring entities", because they exist only on the client side to help
   -- in the authoring of new content.
   local cursor_entity = _client:create_authoring_entity('')
   local mob = cursor_entity:add_component('mob')
   mob:set_interpolate_movement(false)
   
   -- add a render object so the cursor entity gets rendered.
   local cursor_render_entity = _client:create_render_entity(1, cursor_entity)
   local node = h3dRadiantCreateStockpileNode(cursor_render_entity:get_node(), 'stockpile designation')

   local cleanup = function(data)
      data = data and {}
      _client:destroy_authoring_entity(cursor_entity:get_id())
      response:complete(data)
   end

   _client:select_xz_region()
      :progress(function (p0, p1)
            mob:set_location_grid_aligned(p0)
            h3dRadiantResizeStockpileNode(node, p1.x - p0.x + 1, p1.z - p0.z + 1);
         end)
      :done(function (p0, p1)
            local args = {
               faction = 'civ',
               p0 = p0,
               p1 = p1
            }
            _client:call('/modules/server/stonehearth_inventory/create_stockpile', '', args)
                     :always(function(data)
                           cleanup(data)
                        end)
         end)
      :fail(function()
            cleanup()
         end)
end

-- called each time the mouse moves on the client.
function CreateStockpile:_on_mouse_event(e, entity_uri)
   radiant.log.warning('moving stockpile %d, %d', e.x, e.y)
   
   -- query the scene to figure out what's under the mouse cursor
   local s = _client:query_scene(e.x, e.y)
  
   -- s.location contains the address of the terrain block that the mouse
   -- is currently pointing to.  if there isn't one, move the workshop
   -- way off the screen so it won't get rendered.
   local pt = s.location and s.location or Point3(0, -100000, 0)

   -- we want the workbench to be on top of that block, so add 1 to y, then
   -- move the cursor workshop to that location
   pt.y = pt.y + 2
   self._cursor_entity:add_component('mob'):set_location_grid_aligned(pt)

   -- if the mouse button just transitioned to up and we're actually pointing
   -- to a box on the terrain, send a message to the server to create the
   -- entity.  this is done by posting to the correct route.
   if e:up(1) and s.location then
      -- destroy our capture object to release the mouse back to the client.  don't
      -- destroy the authoring object yet!  doing so now will result in a brief period
      -- of time where the server side object has not yet been created, yet the client
      -- authoring object has been destroyed.  that leads to flicker, which is ugly.
      self._capture:destroy()                     

      -- pass "" for the function name so the deafult (handle_request) is
      -- called.  this will return a Deferred object which we can use to track
      -- the call's progress
      _client:call('/modules/server/samplecode_place_workbench/create_workbench', "", { entity = entity_uri, location = pt })
               :always(function ()
                     -- whether the request succeeds or fails, go ahead and destroy
                     -- the authoring entity.  do it after the request returns to avoid
                     -- the ugly flickering that would occur had we destroyed it when
                     -- we uninstalled the mouse cursor
                     _client:destroy_authoring_entity(self._cursor_entity:get_id())
                  end)

   end   

   -- return true to prevent the mouse event from propogating to the UI
   return true
end

return CreateStockpile
