local build_util = require 'lib.build_util'
local StructureEditor = require 'services.client.build_editor.structure_editor'
local PortalEditor = require 'services.client.build_editor.portal_editor'

local log = radiant.log.create_logger('build_editor')

local DoodadPlacer = class()

function DoodadPlacer:__init(build_service)
   self._build_service = build_service
end

function DoodadPlacer:destroy()
   stonehearth.selection:register_tool(self, false)

   if self._invalid_cursor then
      self._invalid_cursor:destroy()
      self._invalid_cursor = nil
   end

   if self._wall_editor then
      self._wall_editor:destroy()
      self._wall_editor = nil
   end

   if self._capture then
      self._capture:destroy()
      self._capture = nil
   end

   local response = self._response
   if response then
      self._response = nil
      response:reject({ error = 'selection cancelled' })
   end
end

function DoodadPlacer:go(session, response, uri)
   local wall_editor
   self._uri = uri
   self._response = response

   self._invalid_cursor = _radiant.client.set_cursor('stonehearth:cursors:invalid_hover')

   stonehearth.selection:register_tool(self, true)

   self._capture = stonehearth.input:capture_input()
                                          :on_mouse_event(function(e)
                                                return self:_on_mouse_event(e)
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
            local fabricator, blueprint, project = build_util.get_fbp_for(entity)
            if blueprint and blueprint:get_component('stonehearth:wall') then

               log:detail('got blueprint %s', tostring(blueprint))
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

         -- the wall editor will submit the response.  nil it out here so we
         -- don't fail it in destroy
         self._response = nil
      end
      self:destroy()
   end

   if stonehearth.selection.user_cancelled(e) then
      self:destroy()
      return true
   end

   local event_consumed = e and (e:down(1) or e:up(1))
   return event_consumed
end

return DoodadPlacer
