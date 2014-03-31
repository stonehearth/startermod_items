local priorities = require('constants').priorities.worker_task
local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3

local CastAoeCallHandler = class()

local all_tasks = {}
local offscreen_pos = Point3(-1000000, 0, -1000000)

function CastAoeCallHandler:client_cast_aoe(session, response, entity, spell)

   self._capture = _radiant.client.capture_input()
   self._capture:on_input(
      function(e)
         if e.type == _radiant.client.Input.MOUSE then
            return self:_on_mouse_event(e.mouse, response, entity, spell)
         elseif e.type == _radiant.client.Input.KEYBOARD then
            return self:_on_keyboard_event(e.keyboard, response)
         end
         return false
      end
   )
end

-- called each time the mouse moves on the client.
function CastAoeCallHandler:_on_mouse_event(e, response, entity, spell)
   assert(self._capture, "got mouse event after releasing capture")

   local range = 20
   local radius = 8
   local s = _radiant.client.query_scene(e.x, e.y)
   -- s.location contains the address of the terrain block that the mouse
   -- is currently pointing to.  if there isn't one, move the cursor
   -- way off the screen so it won't get rendered.
   local pt = s:is_valid() and s:intersection_of(0) or offscreen_pos
   pt.y = pt.y + 1

   local in_range = entity:get_component('mob'):get_world_grid_location():distance_to(pt) <= range
   local material, material_path

   if in_range then
      material_path = "materials/aoe_reticle/aoe_reticle.material.xml"
   else
      material_path = "materials/out_of_range_reticle/out_of_range_reticle.material.xml"
   end

   material = h3dAddResource(H3DResTypes.Material, material_path, 0)

   if not self._cursor_node then
      self._cursor_node = h3dAddProjectorNode(H3DRootNode, "cursornode", material)
   end

   h3dSetNodeParamI(self._cursor_node, H3DProjectorNodeParams.MatResI, material)
   h3dSetNodeTransform(self._cursor_node, pt.x - radius/2, 0, pt.z - radius/2, 0, 0, 0, radius, 1, radius)

   -- if the mouse button just transitioned to up and we're actually pointing
   -- to a box on the terrain, send a message to the server to create the
   -- entity.  this is done by posting to the correct route.
   if e:up(1) and in_range and s.location then
      
      _radiant.call('stonehearth:server_cast_aoe', entity:get_id(), spell, pt)
         :always(
            function ()
               -- whether the request succeeds or fails, go ahead and destroy
               -- the authoring entity.  do it after the request returns to avoid
               -- the ugly flickering that would occur had we destroyed it when
               -- we uninstalled the mouse cursor
               self:_cleanup()
               response:resolve({ result = true })
            end
         )
   end

   -- return true to prevent the mouse event from propogating to the UI
   return true
end

function CastAoeCallHandler:server_cast_aoe(session, response, entity_id, spell, location)
   local entity = radiant.entities.get_entity(entity_id)
   local pt = Point3(location.x, location.y, location.z)

   local spell = 'stonehearth:unit_control:abilities:' .. spell
   local town = stonehearth.town:get_town(session.player_id)
   town:command_unit(entity, spell, { location = pt })
            :once()
            :start()
   return true
end


function CastAoeCallHandler:_on_keyboard_event(e, response)
   if e.key == _radiant.client.KeyboardInput.KEY_ESC and e.down then
      self:_cleanup()
      response:resolve({ result = false })
   end
   return true
end

-- destroy our capture object to release the mouse back to the client.  
function CastAoeCallHandler:_cleanup(e, response)
   if self._capture then
      self._capture:destroy()
      self._capture = nil
   end

   if self._cursor_node then
      h3dRemoveNode(self._cursor_node)
      self._cursor_node = nil
   end
   
end


return CastAoeCallHandler
