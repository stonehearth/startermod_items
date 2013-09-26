local Point3 = _radiant.csg.Point3
local CreateWorkbench = class()

-- client side object to add a new bench to the world.  this method is invoked
-- by POSTing to the route for this file in the manifest.
function CreateWorkbench:choose_workbench_location(session, response, workbench_entity)
   -- create a new "cursor entity".  this is the entity that will move around the
   -- screen to preview where the workbench will go.  these entities are called
   -- "authoring entities", because they exist only on the client side to help
   -- in the authoring of new content.
   self._cursor_entity = radiant.entities.create_entity(workbench_entity)

   -- add a render object so the cursor entity gets rendered.
   local re = _radiant.client.create_render_entity(1, self._cursor_entity)

   -- at this point we could manipulate re to change the way the cursor gets
   -- rendered (e.g. transparent)...


   -- capture the mouse.  Call our _on_mouse_event each time, passing in
   -- the entity that we're supposed to create whenever the user clicks.
   self._capture = _radiant.client.capture_input()
   self._capture:on_input(function(e)
         if e.type == _radiant.client.Input.MOUSE then
            self:_on_mouse_event(e.mouse, workbench_entity, response)
            return true
         end
         return false
      end)
end

-- called each time the mouse moves on the client.
function CreateWorkbench:_on_mouse_event(e, workbench_entity, response)
   assert(self._capture, "got mouse event after releasing capture")
   
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
      self._capture = nil

      -- pass "" for the function name so the deafult (handle_request) is
      -- called.  this will return a Deferred object which we can use to track
      -- the call's progress
      _radiant.call_obj('stonehearth_crafter', 'create_workbench', workbench_entity, pt)
               :always(function ()
                     -- whether the request succeeds or fails, go ahead and destroy
                     -- the authoring entity.  do it after the request returns to avoid
                     -- the ugly flickering that would occur had we destroyed it when
                     -- we uninstalled the mouse cursor
                     _radiant.client.destroy_authoring_entity(self._cursor_entity:get_id())
                     response:resolve({})
                  end)

   end

   -- return true to prevent the mouse event from propogating to the UI
   return true
end

-- server side object to handle creation of the workbench.  this is called
-- by doing a POST to the route for this file specified in the manifest.

function CreateWorkbench:create_workbench(session, response, workbench_entity_uri, pt)
   -- pull the location and entity uri out of the postdata, create that
   -- entity, and move it there.
   local location = Point3(pt.x, pt.y, pt.z)
   local workbench_entity = radiant.entities.create_entity(workbench_entity_uri)
   local workshop_component = workbench_entity:get_component('stonehearth:workshop')

   -- Plasde the bench in the world
   radiant.terrain.place_entity(workbench_entity, location)

   -- Place the promotion talisman on the workbench, if there is one
   local promotion_talisman_entity, outbox_entity = workshop_component:init_from_scratch()

   -- set the faction of the bench and talisman
   workbench_entity:get_component('unit_info'):set_faction(session.faction)
   promotion_talisman_entity:get_component('unit_info'):set_faction(session.faction)
   outbox_entity:get_component('unit_info'):set_faction(session.faction)

   -- return the entity, in case the client cares (e.g. if they want to make that
   -- the selected item)
   return { workbench_entity = workbench_entity }
end

return CreateWorkbench
