local priorities = require('constants').priorities.worker_task
local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3

local UnitControlCallHandler = class()

function UnitControlCallHandler:choose_unit_location(session, response, entity)

   self._entity = entity;
   self._cursor_entity = radiant.entities.create_entity('stonehearth:camp_standard')
   local re = _radiant.client.create_render_entity(1, self._cursor_entity)

   self._capture = _radiant.client.capture_input()
   self._capture:on_input(function(e)
         if e.type == _radiant.client.Input.MOUSE then
            self:_on_mouse_event(e.mouse, response)
            return true
         elseif e.type == _radiant.client.Input.KEYBOARD then
            self:_on_keyboard_event(e.keyboard, response)
            return true
         end
         return false
      end)

   return true
end

-- called each time the mouse moves on the client.
function UnitControlCallHandler:_on_mouse_event(e, response)
   assert(self._capture, "got mouse event after releasing capture")

   local s = _radiant.client.query_scene(e.x, e.y)

   -- s.location contains the address of the terrain block that the mouse
   -- is currently pointing to.  if there isn't one, move the workshop
   -- way off the screen so it won't get rendered.
   local pt = s.location and s.location or Point3(0, -100000, 0)

   pt.y = pt.y + 1
   self._cursor_entity:add_component('mob'):set_location_grid_aligned(pt)

   -- if the mouse button just transitioned to up and we're actually pointing
   -- to a box on the terrain, send a message to the server to create the
   -- entity.  this is done by posting to the correct route.
   if e:up(1) and s.location then
      
      _radiant.call('stonehearth:move_unit', self._entity:get_id(), pt)
               :always(function ()
                     -- whether the request succeeds or fails, go ahead and destroy
                     -- the authoring entity.  do it after the request returns to avoid
                     -- the ugly flickering that would occur had we destroyed it when
                     -- we uninstalled the mouse cursor
                     self:_cleanup()
                     response:resolve({ result = true })
                  end)


      response:resolve({ result = true })
   end

   -- return true to prevent the mouse event from propogating to the UI
   return true
end

function UnitControlCallHandler:move_unit(session, response, entity_id, pt)
   local entity = radiant.entities.get_entity(entity_id)
   radiant.events.trigger(entity, 'stonehearth:move_unit', {
         location = pt
      })
   return true
end


function UnitControlCallHandler:_on_keyboard_event(e, response)
   if e.key == _radiant.client.KeyboardInput.KEY_ESC and e.down then
      self:_cleanup()
      response:resolve({ result = false })
   end
   return true
end

-- destroy our capture object to release the mouse back to the client.  
function UnitControlCallHandler:_cleanup(e, response)
   self._capture:destroy()
   self._capture = nil
   _radiant.client.destroy_authoring_entity(self._cursor_entity:get_id())
end


return UnitControlCallHandler
