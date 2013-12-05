local priorities = require('constants').priorities.worker_task
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
   self._cursor_entity:add_component('render_info')
      :set_material('materials/ghost_item.xml')
      :set_model_mode('blueprint')

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
         if e.type == _radiant.client.Input.KEYBOARD then
            self:_on_keyboard_event(e.keyboard)
         end
         --Don't consume the event in case the UI wants to do something too
         return false
      end)
end

-- called each time the mouse moves on the client.
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

function PlaceItemCallHandler:_on_keyboard_event(e)
   local esc_down = _radiant.client.is_key_down(_radiant.client.KeyboardInput.ESC)
   if esc_down then
      self._capture:destroy()
      self._capture = nil
      _radiant.client.destroy_authoring_entity(self._cursor_entity:get_id())
   end
   return false
end

--- Tell a worker to place the item in the world
-- Server side object to handle creation of the workbench.  This is called
-- by doing a POST to the route for this file specified in the manifest.
function PlaceItemCallHandler:place_item_in_world(session, response, target_entity, full_sized_uri, location, rotation)
   local task = self:_init_pickup_worker_task(session, full_sized_uri, location, rotation)
   task:add_work_object(target_entity)
   task:start()
   return true
end

--- Place any object that matches the entity_uri
-- server side object to handle creation of the workbench.  this is called
-- by doing a POST to the route for this file specified in the manifest.
function PlaceItemCallHandler:place_item_type_in_world(session, response, entity_uri, full_item_uri, location, rotation)
   local task = self:_init_pickup_worker_task(session, full_item_uri, location, rotation)
   local object_filter_fn = function(entity)
      if entity:get_uri() == entity_uri then
         return true
      end
      return false
   end
   task:set_work_object_filter_fn(object_filter_fn)
   task:start()
   return true
end

--- Init all the things common to the pickup_worker_task
-- does everything except set the target and start the task
function PlaceItemCallHandler:_init_pickup_worker_task(session, full_sized_uri, coor_location, rotation)
   --Place the ghost entity first
   local location = Point3(coor_location.x, coor_location.y, coor_location.z)
   local ghost_entity = radiant.entities.create_entity()
   local ghost_entity_component = ghost_entity:add_component('stonehearth:ghost_item')
   ghost_entity_component:set_object_data(full_sized_uri, location, rotation)
   radiant.terrain.place_entity(ghost_entity, location)

   --Summon the worker scheduler
   local ws = radiant.mods.load('stonehearth').worker_scheduler
   local worker_scheduler = ws:get_worker_scheduler(session.faction)

   -- Make a task for picking up the object the user designated
   -- Any worker that's not carrying anything will do...
   local not_carrying_fn = function (worker)
      return radiant.entities.get_carrying(worker) == nil
   end

   local pickup_item_task = worker_scheduler:add_worker_task('placing_item_task')
                  :set_worker_filter_fn(not_carrying_fn)
                  :set_priority(priorities.PLACE_ITEM)

   pickup_item_task:set_action_fn(
      function (path)
         return 'stonehearth:place_item', path, ghost_entity, rotation, pickup_item_task
      end
   )
   return pickup_item_task
end


return PlaceItemCallHandler
