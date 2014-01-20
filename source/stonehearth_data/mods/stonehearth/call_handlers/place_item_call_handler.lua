local priorities = require('constants').priorities.worker_task
local Point3 = _radiant.csg.Point3

local PlaceItemCallHandler = class()
local log = radiant.log.create_logger('call_handlers.place_item')


-- Client side object to place an item in the world. The item exists as an icon first
-- This method is invoked by POSTing to the route for this file in the manifest.
-- TODO: merge/factor out with CreateWorkshop?
function PlaceItemCallHandler:choose_place_item_location(session, response, target_entity, entity_uri)
   --Save whether the entity is just a type or an actual entity to determine if we're 
   --going to place an actual object or just an object type
   assert(target_entity, "Must pass entity data about the object to place")
   self._next_call = nil
   self._target_entity_data = nil
   if type(target_entity) == 'string' then
      self._next_call = 'stonehearth:place_item_type_in_world'
      self._target_entity_data = target_entity
   else
      self._next_call = 'stonehearth:place_item_in_world'
      self._target_entity_data = target_entity:get_id()
   end
   
   -- create a new "cursor entity".  this is the entity that will move around the
   -- screen to preview where the object will go.  these entities are called
   -- "authoring entities", because they exist only on the client side to help
   -- in the authoring of new content.
   -- TODO: show places the item cannot/should not be placed
   self._entity_uri = entity_uri
   self._cursor_entity = radiant.entities.create_entity(entity_uri)

   -- add a render object so the cursor entity gets rendered.
   local re = _radiant.client.create_render_entity(1, self._cursor_entity)
   self._cursor_entity:add_component('render_info')
      :set_material('materials/ghost_item.xml')
      --TODO: Qubicle brush fails on qb files with multiple matrices.
      --See this bug: http://bugs.radiant-entertainment.com:8080/browse/SH-38 
      --:set_model_mode('blueprint')

   log:debug("created render entity")

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
      log:info('Pressed right click')
      self._curr_rotation = self._curr_rotation + 90
      self._curr_rotation = self._curr_rotation % 360
      self._cursor_entity:add_component('mob'):turn_to(self._curr_rotation + 180)
   end

   if e:up(1) and s.location then

      self:_destroy_capture()
      _radiant.call(self._next_call, self._target_entity_data, self._entity_uri, pt, self._curr_rotation+180)
         :done(function (result)
            response:resolve(result)
            end)
         :fail(function(result)
            response:reject(result)
            end)
         :always(function ()
            -- whether the request succeeds or fails, go ahead and destroy
            -- the authoring entity.  Do it after the request returns to avoid
            -- the ugly flickering that would occur had we destroyed it when
            -- we uninstalled the mouse cursor
            _radiant.client.destroy_authoring_entity(self._cursor_entity:get_id())
            end)
   end
   -- return true to prevent the mouse event from propogating to the UI
   return true
end

function PlaceItemCallHandler:_on_keyboard_event(e)
   if e.key == _radiant.client.KeyboardInput.KEY_ESC and e.down then
      self:_destroy_capture()
       _radiant.client.destroy_authoring_entity(self._cursor_entity:get_id())
   end
   return false
end

--- Destroy our capture object to release the mouse back to the client.
function PlaceItemCallHandler:_destroy_capture()
   self._capture:destroy()
   self._capture = nil
end

--- Tell a worker to place the item in the world
-- Server side object to handle creation of the workbench.  This is called
-- by doing a POST to the route for this file specified in the manifest.
function PlaceItemCallHandler:place_item_in_world(session, response, entity_id, full_sized_uri, location, rotation)
   local location = Point3(location.x, location.y, location.z)
   local item = radiant.entities.get_entity(entity_id)

   local ghost_entity = radiant.entities.create_entity()
   local ghost_entity_component = ghost_entity:add_component('stonehearth:ghost_item')
   ghost_entity_component:set_full_sized_mod_uri(full_sized_uri)
   radiant.terrain.place_entity(ghost_entity, location)
   radiant.entities.turn_to(ghost_entity, rotation)

   local remove_ghost_entity = function(placed_item)
      radiant.entities.destroy_entity(ghost_entity)
   end

   local scheduler = stonehearth.tasks:get_scheduler('stonehearth:workers', session.faction)
   scheduler:create_task('stonehearth:place_item', {
         item = item,
         location = location,
         rotation = rotation,
         finish_fn = remove_ghost_entity,
      })
      :once()
      :start()

   return true
end

--- Place any object that matches the entity_uri
-- server side object to handle creation of the workbench.  this is called
-- by doing a POST to the route for this file specified in the manifest.
function PlaceItemCallHandler:place_item_type_in_world(session, response, entity_uri, full_item_uri, location, rotation)
   local location = Point3(location.x, location.y, location.z)

   local ghost_entity = radiant.entities.create_entity()
   local ghost_entity_component = ghost_entity:add_component('stonehearth:ghost_item')
   ghost_entity_component:set_full_sized_mod_uri(full_item_uri)
   radiant.terrain.place_entity(ghost_entity, location)
   radiant.entities.turn_to(ghost_entity, rotation)


   local remove_ghost_entity = function(placed_item)
      radiant.entities.destroy_entity(ghost_entity)
   end
   local filter_fn = function(item)
      return item:get_uri() == entity_uri
   end

   local scheduler = stonehearth.tasks:get_scheduler('stonehearth:workers', session.faction)
   scheduler:create_task('stonehearth:place_item_type', {
         filter_fn = filter_fn,
         location = location,
         rotation = rotation,
         finish_fn = remove_ghost_entity,
      })
      :start()

end


return PlaceItemCallHandler
