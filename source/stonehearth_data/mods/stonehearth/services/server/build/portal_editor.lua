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
                     
   self._wall:add_portal(self._portal_blueprint)
   return self
end

function PortalEditor:on_mouse_event(e, selection)
   local location
   local proxy_fabricator = self:get_proxy_fabricator()
   for result in selection:each_result() do
      if result.entity == proxy_fabricator then
         location = result.brick
      end
   end
   if location then
      location = location - self:get_world_origin()      
      location.y = 0
      location[self._wall:get_normal_coord()] = 0
      radiant.entities.move_to(self._portal_blueprint, location)
      self._wall:layout()
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
