local Point3 = _radiant.csg.Point3

local StructureEditor = require 'services.server.build.structure_editor'
local WallEditor = class(StructureEditor)

local log = radiant.log.create_logger('build_editor')

function WallEditor:__init(fabricator, blueprint, project)
   self[StructureEditor]:__init(fabricator, blueprint, project, 'stonehearth:wall')
   self._wall = self:get_proxy_blueprint():get_component('stonehearth:wall')
end

function WallEditor:destroy()  
   if self._portal then
      log:detail('destroying portal %s', tostring(self._portal))
      radiant.entities.destroy_entity(self._portal)
      self._portal = nil
   end
   self[StructureEditor]:destroy()
end

function WallEditor:set_portal_uri(uri)
   self._portal_uri = uri
   self._portal = radiant.entities.create_entity(uri)
   self._portal:add_component('render_info')
                  :set_material('materials/ghost_item.xml')

   self._wall:add_portal(self._portal)
   return self
end

function WallEditor:on_mouse_event(e, selection)
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
      radiant.entities.move_to(self._portal, location)
      self._wall:layout()
   end      
end

function WallEditor:submit(response)   
   local location = self._portal:get_component('mob'):get_grid_location()
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

return WallEditor
