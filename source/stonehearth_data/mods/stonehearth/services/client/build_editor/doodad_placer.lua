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
   if stonehearth.selection.user_cancelled(e) then
      self:destroy()
      return true
   end

   local raycast_results = _radiant.client.query_scene(e.x, e.y)
   local entity, fabricator, blueprint, project
   local found_wall = false

   -- look for a valid wall
   for result in raycast_results:each_result() do
      entity = result.entity
      local ignore_entity = self:_ignore_entity(entity)

      if not ignore_entity then
         log:detail('hit entity %s', entity)

         fabricator, blueprint, project = build_util.get_fbp_for(entity)

         if blueprint then
            found_wall = blueprint:get_component('stonehearth:wall')
            break
         end
      end
   end

   local keep_editor = found_wall and self._wall_editor and self._wall_editor:should_keep_focus(entity)

   if not keep_editor then
      if self._wall_editor then
         log:detail('destroying wall editor')
         self._wall_editor:destroy()
         self._wall_editor = nil
      end
   end

   if found_wall then
      if not self._wall_editor then
         log:detail('creating wall editor for blueprint: %s', blueprint)
         self._wall_editor = PortalEditor(self._build_service)
                              :begin_editing(fabricator, blueprint, project)
                              :set_fixture_uri(self._uri)
                              :go()
      end

      self._wall_editor:on_mouse_event(e, raycast_results)
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

   local event_consumed = e and (e:down(1) or e:up(1))
   return event_consumed
end

function DoodadPlacer:_ignore_entity(entity)
   if not entity or not entity:is_valid() then
      return true
   end

   if not stonehearth.selection:is_selectable(entity) then
      return true
   end

   return false
end

return DoodadPlacer
