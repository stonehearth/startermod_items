local priorities = require('constants').priorities.worker_task
local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3

local MoveUnitCallHandler = class()

function MoveUnitCallHandler:client_move_unit(session, response, entity)

   self._entity = entity;
   self._cursor_entity = radiant.entities.create_entity('stonehearth:camp_standard')
   local re = _radiant.client.create_render_entity(1, self._cursor_entity)

   self._input_capture = stonehearth.input:capture_input()
                           :on_mouse_event(function(e)
                                 return self:_on_mouse_event(e, response)
                              end)
                           :on_keyboard_event(function(e)
                                 return self:_on_keyboard_event(e, response)
                              end)

   return true
end

-- called each time the mouse moves on the client.
function MoveUnitCallHandler:_on_mouse_event(e, response)
   assert(self._input_capture, "got mouse event after releasing capture")

   local s = _radiant.client.query_scene(e.x, e.y)

   -- s.location contains the address of the terrain block that the mouse
   -- is currently pointing to.  if there isn't one, move the workshop
   -- way off the screen so it won't get rendered.
   local pt = s:is_valid() and s:brick_of(0) or Point3(0, -100000, 0)

   pt.y = pt.y + 1
   self._cursor_entity:add_component('mob'):set_location_grid_aligned(pt)

   -- if the mouse button just transitioned to up and we're actually pointing
   -- to a box on the terrain, send a message to the server to create the
   -- entity.  this is done by posting to the correct route.
   if e:up(1) and s:is_valid() then     
      _radiant.call('stonehearth:server_move_unit', self._entity:get_id(), pt)
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

function MoveUnitCallHandler:server_move_unit(session, response, entity_id, location)
   local entity = radiant.entities.get_entity(entity_id)
   local pt = Point3(location.x, location.y, location.z)

   if not entity:get_component('stonehearth:ai') then
      return
   end
   
   local town = stonehearth.town:get_town(session.player_id)
   town:command_unit(entity, 'stonehearth:unit_control:move_unit', { location = pt })
            :once()
            :start()
   return true
end


function MoveUnitCallHandler:_on_keyboard_event(e, response)
   if e.key == _radiant.client.KeyboardInput.KEY_ESC and e.down then
      self:_cleanup()
      response:resolve({ result = false })
   end
   return true
end

-- destroy our capture object to release the mouse back to the client.  
function MoveUnitCallHandler:_cleanup(e, response)
   if self._input_capture then
      self._input_capture:destroy()
      self._input_capture = nil
   end

   if self._cursor_entity then   
      _radiant.client.destroy_authoring_entity(self._cursor_entity:get_id())
      self._cursor_entity = nil
   end
end

return MoveUnitCallHandler
