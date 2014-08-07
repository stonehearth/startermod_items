local StructureEditor = require 'services.client.build_editor.structure_editor'
local PortalEditor = require 'services.client.build_editor.portal_editor'

local log = radiant.log.create_logger('build_editor')

local DoodadPlacer = class()

function DoodadPlacer:__init(build_service)
   self._build_service = build_service
end

function DoodadPlacer:deactivate_tool()
   self._response:reject('finished')

   self:destroy()
end

function DoodadPlacer:destroy()
   stonehearth.selection:register_tool(self, false)

   if self._wall_editor then
      self._wall_editor:destroy()
      self._wall_editor = nil
   end

   self._response = nil

   if self._capture then
      self._capture:destroy()
      self._capture = nil
   end
end

function DoodadPlacer:go(session, response, uri)
   local wall_editor
   self._uri = uri
   self._response = response
   self._capture = stonehearth.input:capture_input()

   stonehearth.selection:register_tool(self, true)

   self._capture:on_mouse_event(function(e)
         return self:_on_mouse_event(e)
         end) 
      :on_keyboard_event(function(e)
         return self:_on_keyboard_event(e)
         end)
   return self
end

function DoodadPlacer:_on_mouse_event(e)
   local s = _radiant.client.query_scene(e.x, e.y)
   if s and s:get_result_count() > 0 then            
      local entity = s:get_result(0).entity
      if entity and entity:is_valid() then
         log:detail('hit entity %s', entity)
         if self._wall_editor and not self._wall_editor:should_keep_focus(entity) then
            log:detail('destroying wall editor')
            self._wall_editor:destroy()
            self._wall_editor = nil
         end
         if not self._wall_editor then
            local fabricator, blueprint, project = stonehearth.build_editor:get_fbp_for_structure(entity, 'stonehearth:wall')
            log:detail('got blueprint %s', tostring(blueprint))
            if blueprint then
               log:detail('creating wall editor for blueprint: %s', blueprint)
               self._wall_editor = PortalEditor(self._build_service)
                                 :begin_editing(fabricator, blueprint, project)
                                 :set_fixture_uri(self._uri)
                                 :go()
            end
         end
         if self._wall_editor then
            self._wall_editor:on_mouse_event(e, s)
         end
      end
   end
   if e:up(1) then
      if self._wall_editor then
         self._wall_editor:submit(self._response)

         -- No need to destroy, because the deferred response (submitted above)
         -- will call self:destroy().  Surprise!
         self._wall_editor = nil
      end
      self:destroy()
   end

   if e:up(2) and not e.dragging then
      self:deactivate_tool()
   end
   return true
end

function DoodadPlacer:_on_keyboard_event(e)
   if e.key == _radiant.client.KeyboardInput.KEY_ESC and e.down then
      self:deactivate_tool()
   end
   return false
end

return DoodadPlacer
