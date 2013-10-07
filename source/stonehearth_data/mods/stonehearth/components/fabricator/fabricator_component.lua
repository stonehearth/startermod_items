local Fabricator = require 'components.fabricator.fabricator'
local FabricatorComponent = class()
local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local Cube3 = _radiant.csg.Cube3
local Color4 = _radiant.csg.Color4

-- this is the component which manages the fabricator entity.
function FabricatorComponent:__init(entity, data_binding)
   self._entity = entity
   self._data_binding = data_binding
end

function FabricatorComponent:add_block(material, location)
   self._fabricator:add_block(material, location)
end

function FabricatorComponent:start_project(name, location, blueprint)
   self.name = name
   radiant.log.info('starting project %s', name)
   
   self._fabricator = Fabricator(name, self._entity, blueprint)
   
   -- finally, put it on the ground.  we also need to put ourselves on the ground
   -- so the pathfinder can find it's way to regions which need to be constructed
   local project = self._fabricator:get_project()
   radiant.terrain.place_entity(project, location)
   radiant.terrain.place_entity(self._entity, location)

   local needs_scaffolding = radiant.entities.get_entity_data(blueprint, 'stonehearth:fabricator').type == 'wall'
   local normal = Point3(1, 0, 0) -- pull off the wall component
   if needs_scaffolding then -- needs scaffolding?
      self:_add_scaffolding_to_project(location, project, normal)
   end
   return self
end


function FabricatorComponent:_add_scaffolding_to_project(location, project, normal)
   -- create a scaffolding blueprint and point it to the project
   local uri = 'stonehearth.scaffolding'
   local scaffolding = radiant.entities.create_entity(uri)
   scaffolding:add_component('stonehearth:scaffolding'):support_project(project, normal)
   radiant.entities.set_faction(scaffolding, project)

   -- create a fabricator entity to build the scaffolding
   local name = string.format('scaffolding for %s', self.name)
   local fabricator = radiant.entities.create_entity()
   fabricator:set_debug_text('fabricator for scaffolding of ' .. project:get_debug_text())
   fabricator:add_component('stonehearth:fabricator')
                              :start_project(name, location, scaffolding)
                              :set_debug_color(Color4(255, 192, 0, 128))
end

function FabricatorComponent:set_debug_color(c)
   self._fabricator:set_debug_color(c)
end

return FabricatorComponent
