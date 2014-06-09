local Point3 = _radiant.csg.Point3

local StructureEditor = require 'services.server.build.structure_editor'
local PortalEditor = class(StructureEditor)

local log = radiant.log.create_logger('build_editor')

function PortalEditor:destroy()
   if self._portal_blueprint then
      radiant.entities.destroy_entity(self._portal_blueprint)
      self._portal_blueprint = nil
   end

   self[StructureEditor]:destroy()
end

function PortalEditor:begin_editing(fabricator, blueprint, project, structure_type)
   self[StructureEditor]:begin_editing(fabricator, blueprint, project, 'stonehearth:wall')
   
   self._wall = self:get_proxy_blueprint():get_component('stonehearth:wall')
   return self
end

function PortalEditor:set_portal_uri(portal_uri)
   local data = radiant.entities.get_entity_data(portal_uri, 'stonehearth:ghost_item')

   self._portal_uri = portal_uri
   self._portal_blueprint_uri = data and data.uri or portal_uri

   return self
end

function PortalEditor:go()
   self._portal_blueprint = radiant.entities.create_entity(self._portal_blueprint_uri)
   self._portal_blueprint:add_component('render_info')
                              :set_material('materials/ghost_item.xml')
   self._cursor_uri = self._portal_blueprint:get_component('stonehearth:fixture')
                                                      :get_cursor()
   if not self._cursor_uri then
      self._cursor_uri = 'stonehearth:cursors:arrow'
   end

   self._portal_blueprint_render_entity = _radiant.client.create_render_entity(1, self._portal_blueprint)
   self._portal_blueprint_render_entity:set_visible_override(false)
   
   self._wall:add_portal(self._portal_blueprint)
   
   return self
end

function PortalEditor:on_mouse_event(e, selection)
   local location
   local proxy_fabricator = self:get_proxy_fabricator()
   for result in selection:each_result() do
      if result.entity == proxy_fabricator then
         location = result.brick
         break
      end
   end
   if location then
      self:_position_fixture(location)
   end
end

function PortalEditor:_position_fixture(location)
   -- convert to local coordinates and clip to the margin
   location = location - self:get_world_origin()
   location[self._wall:get_normal_coord()] = 0
   
   local location = self:get_blueprint()
                        :get_component('stonehearth:wall')
                           :compute_fixture_placement(self._portal_blueprint, location)

   self._portal_blueprint_render_entity:set_visible_override(location ~= nil)
   if location then
      self:_change_cursor(self._cursor_uri)
      radiant.entities.move_to(self._portal_blueprint, location)
      self._wall:layout()
   else
      self._change_cursor('stonehearth:cursors:invalid_hover')
   end
end

function PortalEditor:_change_cursor(uri)
   if self._installed_cursor_uri ~= uri then
      if self._cursor then
         self._cursor:destroy()         
         self._cursor = nil
      end
      if uri then
         self._cursor = _radiant.client.set_cursor(uri)
      end
      self._installed_cursor_uri = uri
   end
end

function PortalEditor:submit(response)   
   local location = self._portal_blueprint:get_component('mob'):get_grid_location()
   _radiant.call('stonehearth:add_portal', self:get_blueprint(), self._portal_uri, location)
      :done(function(r)
            response:resolve(r)
         end)
      :fail(function(r)
            response:reject(r)
         end)
      :always(function()
            self:destroy()
         end)
end

return PortalEditor
