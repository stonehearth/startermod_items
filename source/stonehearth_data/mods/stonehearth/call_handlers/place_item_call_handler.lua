local Point3 = _radiant.csg.Point3
local PlaceItemCallHandler = class()

-- Client side object to place an item in the world. The item exists as an icon first
-- This method is invoked by POSTing to the route for this file in the manifest.
-- TODO: merge/factor out with CreateWorkshop?
function PlaceItemCallHandler:choose_place_item_location(session, response, entity_uri)
   -- create a new "cursor entity".  this is the entity that will move around the
   -- screen to preview where the object will go.  these entities are called
   -- "authoring entities", because they exist only on the client side to help
   -- in the authoring of new content.
   -- TODO: show places the item cannot/should not be placed

   self._cursor_entity = radiant.entities.create_entity(entity_uri)

   -- add a render object so the cursor entity gets rendered.
   local re = _radiant.client.create_render_entity(1, self._cursor_entity)

   radiant.log.info("created render entity")

   -- set self._curr_rotation to 0, since there has been no rotation for this object yet
   self._curr_rotation = 0

   -- at this point we could manipulate re to change the way the cursor gets
   -- rendered (e.g. transparent)...

   -- capture the mouse.  Call our _on_mouse_event each time, passing in
   -- the entity that we're supposed to create whenever the user clicks.
   self._capture = _radiant.client.capture_input()
   self._capture:on_input(function(e)
         if e.type == _radiant.client.Input.MOUSE then
            self:_on_mouse_event(e.mouse, response)
            return true
         end
         return false
      end)
end

-- called each time the mouse moves on the client.
-- TODO: FIX THIS TO THE NEW TASK MODEL BEFORE PUSHING!!!!
function PlaceItemCallHandler:_on_mouse_event(e, response)
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

   --test for mouse right-click
   if e:up(2) and s.location then
      radiant.log.info('Pressed right click')
      self._curr_rotation = self._curr_rotation + 90
      self._curr_rotation = self._curr_rotation % 360
      self._cursor_entity:add_component('mob'):turn_to(self._curr_rotation + 180)
   end

   if e:up(1) and s.location then
      -- destroy our capture object to release the mouse back to the client.  don't
      -- destroy the authoring object yet!  doing so now will result in a brief period
      -- of time where the server side object has not yet been created, yet the client
      -- authoring object has been destroyed.  that leads to flicker, which is ugly.
      self._capture:destroy()
      self._capture = nil

      response:resolve({
         location = pt,
         rotation = self._curr_rotation + 180
      })
      _radiant.client.destroy_authoring_entity(self._cursor_entity:get_id())
   end

   -- return true to prevent the mouse event from propogating to the UI
   return true
end

--- Tell a worker to place the item in the world
-- Server side object to handle creation of the workbench.  This is called
-- by doing a POST to the route for this file specified in the manifest.
function PlaceItemCallHandler:place_item_in_world(session, response, proxy_entity, location, rotation)
   -- TODO: BUGS with multiple workers running for the same object
   local point3_location = Point3(location.x, location.y, location.z)
   self:_init_pickup_worker_task(session, proxy_entity, point3_location, rotation)

   --[[
   -- TODO: Use this code to immediately put down a ghost version of the item (half opacity)
   -- and swap to "building" mode
   -- TODO: remove it when the task is complete
   -- Reuse the code below, but attach the ghostly object to the proxy, so we can remove
   -- it later.
   -- pull the location and entity uri out of the postdata, create that
   -- entity, and move it there.
   local location = Point3(location.x, location.y, location.z)
   --local item_entity = radiant.entities.create_entity(postdata.item_uri)
   radiant.log.info('in server, placing item in the world!!! %s', tostring(proxy_entity))

   local full_sized_entity = proxy_entity:get_component('stonehearth:placeable_item_proxy'):get_full_sized_entity()

   -- Remove the icon (TODO: what about stacked icons? Remove just one of the stack?)
   radiant.terrain.remove_entity(proxy_entity)

   -- Place the item in the world
   radiant.terrain.place_entity(full_sized_entity, location)
   radiant.entities.turn_to(full_sized_entity, rotation)

   --item_entity:get_component('unit_info'):set_faction(session.faction)
   return { full_sized_entity = full_sized_entity }
   ]]
end

function PlaceItemCallHandler:_init_pickup_worker_task(session, proxy_entity, location, rotation)
   local ws = radiant.mods.load('stonehearth').worker_scheduler
   local worker_scheduler = ws:get_worker_scheduler(session.faction)


   --Task for picking up the object the user designated
   -- Any worker that's not carrying anything will do...
   local not_carrying_fn = function (worker)
      return radiant.entities.get_carrying(worker) == nil
   end

   local pickup_item_task = worker_scheduler:add_worker_task('placing_item_task')
                  :set_worker_filter_fn(not_carrying_fn)
                  :add_work_object(proxy_entity)

   pickup_item_task:set_action_fn(
      function (path)
         return 'stonehearth:place_item', path, location, rotation, pickup_item_task
      end
   )

   pickup_item_task:start()
end

-- server side object to handle creation of the workbench.  this is called
-- by doing a POST to the route for this file specified in the manifest.
function PlaceItemCallHandler:place_item_type_in_world(session, response, full_sized_entity_uri, location, rotation)
   -- xxx: we really want a worker to just find one, but this is ok for now.__call
   return true
end

return PlaceItemCallHandler
