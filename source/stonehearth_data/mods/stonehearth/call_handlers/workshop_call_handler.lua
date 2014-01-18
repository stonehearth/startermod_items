local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Color3 = _radiant.csg.Color3
local Rect2 = _radiant.csg.Rect2
local Point2 = _radiant.csg.Point2
local WorkshopCallHandler = class()
local log = radiant.log.create_logger('call_handlers.worker')
local priorities = require('constants').priorities.worker_task
local IngredientList = require 'components.workshop.ingredient_list'


--- Client side object to add a new bench to the world.  this method is invoked
--  by POSTing to the route for this file in the manifest.
function WorkshopCallHandler:choose_workbench_location(session, response, workbench_entity)
   -- create a new "cursor entity".  This is the entity that will move around the
   -- screen to preview where the workbench will go.  these entities are called
   -- "authoring entities", because they exist only on the client side to help
   -- in the authoring of new content.
   self._cursor_entity = radiant.entities.create_entity(workbench_entity)

   -- add a render object so the cursor entity gets rendered.
   local re = _radiant.client.create_render_entity(1, self._cursor_entity)
   self._cursor_entity:add_component('render_info')
      :set_material('materials/ghost_item.xml')

   --Set this to zero, since there has been no rotation yet
   self._curr_rotation = 0
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
         if e.type == _radiant.client.Input.KEYBOARD then
            self:_on_keyboard_event(e.keyboard, response)
         end
         --Don't consume the event in case the UI wants to do something too
         return false
      end)
end

-- Called each time the mouse moves on the client.
function WorkshopCallHandler:_on_mouse_event(e, workbench_entity, response)
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

   --test for mouse right-click
   if e:up(2) and s.location then
      log:info('Pressed right click')
      self._curr_rotation = self._curr_rotation + 90
      self._curr_rotation = self._curr_rotation % 360
      self._cursor_entity:add_component('mob'):turn_to(self._curr_rotation + 180)
   end

   -- if the mouse button just transitioned to up and we're actually pointing
   -- to a box on the terrain, send a message to the server to create the ghost
   -- entity.  this is done by posting to the correct route.
   if e:up(1) and s.location then
      -- destroy our capture object to release the mouse back to the client.  don't
      -- destroy the authoring object yet!  doing so now will result in a brief period
      -- of time where the server side object has not yet been created, yet the client
      -- authoring object has been destroyed.  that leads to flicker, which is ugly.
      self._capture:destroy()
      self._capture = nil

      -- pass "" for the function name so the deafult (handle_request) is
      -- called.  this will return a Deferred object which we can use to track
      -- the call's progress
      _radiant.call('stonehearth:create_ghost_workbench', workbench_entity, pt, self._curr_rotation + 180)
               :done(function (result)
                     response:resolve(result)
                  end)
               :fail(function(result)
                     response:reject(result)
                  end)
               :always(function ()
                     -- whether the request succeeds or fails, go ahead and destroy
                     -- the authoring entity.  do it after the request returns to avoid
                     -- the ugly flickering that would occur had we destroyed it when
                     -- we uninstalled the mouse cursor
                     _radiant.client.destroy_authoring_entity(self._cursor_entity:get_id())
                  end)

   end
   -- return true to prevent the mouse event from propogating to the UI
   return true
end

--- When placing a workbench, if a key is pressed, call this function
function WorkshopCallHandler:_on_keyboard_event(e, response)
   if e.key == _radiant.client.KeyboardInput.KEY_ESC and e.down then
      self:_destroy_capture()
      if self._cursor_entity then
         _radiant.client.destroy_authoring_entity(self._cursor_entity:get_id())
      end
       response:resolve({})
   end
   return false
end

--- Destroy our capture object to release the mouse back to the client.
function WorkshopCallHandler:_destroy_capture()
   self._capture:destroy()
   self._capture = nil
end

--- When placing an outbox, if a key is pressed, call this function
--  Note: we don't need to destroy the capture becase the cleanup function already does.
function WorkshopCallHandler:_on_outbox_keyboard_event(e, ghost_entity, response)
   if e.key == _radiant.client.KeyboardInput.KEY_ESC and e.down then
      _radiant.call('stonehearth:destroy_ghost_workbench', ghost_entity:get_id())
      response:resolve({cancelled = true})
   end
   return false   
end

--- Create a shadow of the workbench.
--  Workers are not actually asked to create the workbench until the outbox is placed too. 
function WorkshopCallHandler:create_ghost_workbench(session, response, workbench_entity_uri, pt, rotation)
   local location = Point3(pt.x, pt.y, pt.z)
   local ghost_entity = radiant.entities.create_entity()
   local ghost_entity_component = ghost_entity:add_component('stonehearth:ghost_item')
   radiant.entities.set_faction(ghost_entity, session.faction)
   ghost_entity_component:set_full_sized_mod_uri(workbench_entity_uri)
   radiant.terrain.place_entity(ghost_entity, location)
   radiant.entities.turn_to(ghost_entity, rotation)
   return {
      workbench_entity = ghost_entity
   }
end

--- Destroys a shadow of a workbench
function WorkshopCallHandler:destroy_ghost_workbench(session, response, ghost_entity_id)
   local ghost_entity = radiant.entities.get_entity(ghost_entity_id)
   radiant.entities.destroy_entity(ghost_entity)
end


--- Client side object to add the workbench's outbox to the world. 
function WorkshopCallHandler:choose_outbox_location(session, response, workbench_entity)

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
   local stockpile_cursor = _radiant.client.set_cursor('stonehearth:cursors:create_stockpile')

   -- capture the keyboard.
   self._capture = _radiant.client.capture_input()
   self._capture:on_input(function(e)
         if e.type == _radiant.client.Input.KEYBOARD then
            self:_on_outbox_keyboard_event(e.keyboard, workbench_entity, response)
         end
         --Don't consume the event in case the UI wants to do something too
         return false
      end)

   local cleanup = function()
      if node then
         h3dRemoveNode(node)
      end
      stockpile_cursor:destroy()
      _radiant.client.destroy_authoring_entity(cursor_entity:get_id())
      self:_destroy_capture()
   end

   _radiant.client.select_xz_region()
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
            node = _radiant.client.create_designation_node(parent_node, self._region:get(), Color3(0, 153, 255), Color3(0, 153, 255));
         end)
      :done(function (box)
            local size = {
               box.max.x - box.min.x + 1,
               box.max.z - box.min.z + 1,
            }
            _radiant.call('stonehearth:create_outbox', box.min, size, workbench_entity:get_id())
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

--- Create the outbox the user specified and tell a worker to build the workbench
function WorkshopCallHandler:create_outbox(session, response, location, size, ghost_workshop_entity_id)
   local outbox_entity = radiant.entities.create_entity('stonehearth:workshop_outbox')
   radiant.terrain.place_entity(outbox_entity, location)
   outbox_entity:get_component('unit_info'):set_faction(session.faction)

   local outbox_component = outbox_entity:get_component('stonehearth:stockpile')
   outbox_component:set_size(size)
   outbox_component:set_outbox(true)

   local ghost_workshop = radiant.entities.get_entity(ghost_workshop_entity_id)

   -- todo: stick this in a taskmaster manager somewhere so we can show it (and cancel it!)
   local CreateWorkshopTaskMaster = require 'services.tasks.task_masters.create_workshop'   
   local scheduler = stonehearth.tasks:get_scheduler('stonehearth:workers', session.faction)
   local task_master = CreateWorkshopTaskMaster(scheduler, ghost_workshop, outbox_entity, session.faction)
   task_master:start()
   return true
end

return WorkshopCallHandler
