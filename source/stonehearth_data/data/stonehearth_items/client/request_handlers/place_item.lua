local Point3 = _radiant.csg.Point3
local PlaceItem = class()

-- Client side object to place an item in the world. The item exists as an icon first
-- This method is invoked by POSTing to the route for this file in the manifest.
-- TODO: merge/factor out with CreateWorkshop?
function PlaceItem:handle_request(query, postdata, response)
   -- create a new "cursor entity".  this is the entity that will move around the
   -- screen to preview where the object will go.  these entities are called
   -- "authoring entities", because they exist only on the client side to help
   -- in the authoring of new content.
   -- TODO: show places the item cannot/should not be placed

   local item_name = query.target_name
   local item_mod = query.target_mod

   --TODO: I don't know why radiant.entities.create_entity(item_mod, item_name) doesn't work here...
   self._cursor_entity = radiant.entities.create_entity('entity(' .. item_mod .. ', ' .. item_name ..')')

   -- add a render object so the cursor entity gets rendered.
   local re = _radiant.client.create_render_entity(1, self._cursor_entity)

   radiant.log.info("created render entity")

   -- at this point we could manipulate re to change the way the cursor gets
   -- rendered (e.g. transparent)...

   -- capture the mouse.  Call our _on_mouse_event each time, passing in
   -- the entity that we're supposed to create whenever the user clicks.
   self._capture = _radiant.client.capture_input()
   self._capture:on_input(function(e)
         if e.type == _radiant.client.MOUSE_INPUT then
            self:_on_mouse_event(e, query.proxy_entity_id, response)
            return true
         end
         return false
      end)
end

-- called each time the mouse moves on the client.
function PlaceItem:_on_mouse_event(e, proxy_id, response)
   -- query the scene to figure out what's under the mouse cursor
   local s = _radiant.client.query_scene(e.x, e.y)

   -- s.location contains the address of the terrain block that the mouse
   -- is currently pointing to.  if there isn't one, move the workshop
   -- way off the screen so it won't get rendered.
   local pt = s.location and s.location or Point3(0, -100000, 0)

   -- we want the workbench to be on top of that block, so add 1 to y, then
   -- move the cursor workshop to that location
   pt.y = pt.y + 1
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

      radiant.log.info('about to call server!!!')
      --'/stonehearth_carpenter_class/entities/carpenter_workbench'

      -- pass "" for the function name so the deafult (handle_request) is
      -- called.  this will return a Deferred object which we can use to track
      -- the call's progress
      _radiant.call('stonehearth_items', 'place_item_server', proxy_id, pt)
               :always(function ()
                     -- whether the request succeeds or fails, go ahead and destroy
                     -- the authoring entity.  do it after the request returns to avoid
                     -- the ugly flickering that would occur had we destroyed it when
                     -- we uninstalled the mouse cursor
                     _radiant.client.destroy_authoring_entity(self._cursor_entity:get_id())
                     response:complete({})
                  end)

   end

   -- return true to prevent the mouse event from propogating to the UI
   return true
end

return PlaceItem
