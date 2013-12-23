local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Color3 = _radiant.csg.Color3
local Rect2 = _radiant.csg.Rect2
local Point2 = _radiant.csg.Point2
local WorkshopCallHandler = class()
local log = radiant.log.create_logger('call_handlers.wor')
local priorities = require('constants').priorities.worker_task

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

--- Create a shadow of the workbench.
--  Workers are not actually asked to create the workbench until the outbox is placed too. 
function WorkshopCallHandler:create_ghost_workbench(session, response, workbench_entity_uri, pt, rotation)
   local location = Point3(pt.x, pt.y, pt.z)
   local ghost_entity = radiant.entities.create_entity()
   local ghost_entity_component = ghost_entity:add_component('stonehearth:ghost_item')
   ghost_entity_component:set_object_data(workbench_entity_uri, location, rotation)
   radiant.terrain.place_entity(ghost_entity, location)
   return {
      workbench_entity = ghost_entity
   }
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

   local cleanup = function()
      if node then
         h3dRemoveNode(node)
      end
      stockpile_cursor:destroy()
      _radiant.client.destroy_authoring_entity(cursor_entity:get_id())
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
function WorkshopCallHandler:create_outbox(session, response, location, size, workbench_entity_id)
   local outbox_entity = radiant.entities.create_entity('stonehearth:workshop_outbox')
   radiant.terrain.place_entity(outbox_entity, location)
   outbox_entity:get_component('unit_info'):set_faction(session.faction)
   local outbox_component = outbox_entity:get_component('stonehearth:stockpile')
   outbox_component:set_size(size)
   outbox_component:set_outbox(true)
   self:_start_worker_create_task(session, outbox_entity, workbench_entity_id)
   return true
end

--- Create the worker task that will build the outbox. 
function WorkshopCallHandler:_start_worker_create_task(session, outbox_entity, workbench_entity_id)
   local ghost_entity = radiant.entities.get_entity(workbench_entity_id)

   --Summon the worker scheduler
   local ws = radiant.mods.load('stonehearth').worker_scheduler
   local worker_scheduler = ws:get_worker_scheduler(session.faction)

   -- Who can do this task? Any worker that's not carrying anything will do...
   local not_carrying_fn = function (worker)
      return radiant.entities.get_carrying(worker) == nil
   end

   --What object is suitable to build a workshop? By default, wood. 
   --TODO: Right now, all workshops are built with 1 object.
   --Fix worker action so it can build with multiple objects
   local ghost_object_data = ghost_entity:get_component('stonehearth:ghost_item'):get_object_data()
   local real_item_uri = ghost_object_data.full_sized_mod_url
   local json = radiant.resources.load_json(real_item_uri)
   local workshop_material = 'wood resource'
   if json and json.components then
      workshop_material = json.components['stonehearth:workshop'].ingredients.material
   end

   local object_filter_fn = function(entity)
      local material = entity:get_component('stonehearth:material')
      if material and material:is(workshop_material) then
         return true
      end
      return false
   end

   --Actually create the worker task 
   local place_workshop_task = worker_scheduler:add_worker_task('placing_workshop_task')
      :set_worker_filter_fn(not_carrying_fn)
      :set_priority(priorities.PLACE_WORKSHOP)
      :set_work_object_filter_fn(object_filter_fn)

   place_workshop_task:set_action_fn(
      function(path)
         return 'stonehearth:place_workshop', path, ghost_entity, outbox_entity, place_workshop_task
      end)

   place_workshop_task:start()
end


return WorkshopCallHandler
