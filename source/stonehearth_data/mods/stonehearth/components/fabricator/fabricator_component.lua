local Fabricator = require 'components.fabricator.fabricator'
local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local Cube3 = _radiant.csg.Cube3
local Color4 = _radiant.csg.Color4
local log = radiant.log.create_logger('commands')

local FabricatorComponent = class()
local log = radiant.log.create_logger('fabricator')

-- this is the component which manages the fabricator entity.
function FabricatorComponent:initialize(entity, json)
   self._entity = entity
end

function FabricatorComponent:destroy()
   log:debug('destroying fabricator component for %s', self._entity)
   
   if self._fabricator then
      self._fabricator:destroy()
      self._fabricator = nil
   end
   if self._scaffolding_blueprint then
      radiant.entities.destroy_entity(self._scaffolding_blueprint)
      self._scaffolding_blueprint = nil
   end
   if self._scaffolding_fabricator then
      radiant.entities.destroy_entity(self._scaffolding_fabricator)
      self._scaffolding_fabricator = nil
   end
end

function FabricatorComponent:add_block(material, location)
   return self._fabricator:add_block(material, location)
end

function FabricatorComponent:find_another_block(material, location)
   return self._fabricator:find_another_block(material, location)
end

function FabricatorComponent:remove_block(location)
   return self._fabricator:remove_block(location)
end

function FabricatorComponent:release_block(location)
   return self._fabricator:release_block(location)
end

function FabricatorComponent:start_project(name, blueprint)
   self.name = name and name or '-- unnamed --'
   log:debug('starting project %s', self.name)
   
   self._fabricator = Fabricator(name, self._entity, blueprint)
   
   -- finally, put it on the ground.  we also need to put ourselves on the ground
   -- so the pathfinder can find it's way to regions which need to be constructed
   local project = self._fabricator:get_project()

   local ci = blueprint:get_component('stonehearth:construction_data')
   if ci:needs_scaffolding() then
      local normal = ci:get_normal()
      assert(radiant.util.is_a(normal, Point3))
      self:_add_scaffolding_to_project(project, blueprint, normal)
   end

   -- remember the blueprint and project 
   self._project = project
   self._blueprint = blueprint
   
   self.__saved_variables:set_data({
      project = self._project,
      blueprint = self._blueprint,
   })
   
   -- stick the fabricator entity in the blueprint and projects, too
   blueprint:add_component('stonehearth:construction_data'):set_fabricator_entity(self._entity)
   project:add_component('stonehearth:construction_data'):set_fabricator_entity(self._entity)

   return self
end

function FabricatorComponent:get_blueprint()
   return self._blueprint
end

function FabricatorComponent:get_project()
   return self._project
end

function FabricatorComponent:_add_scaffolding_to_project(project, blueprint, normal)
   -- create a scaffolding blueprint and point it to the project
   local uri = 'stonehearth:scaffolding'
   local transform = project:add_component('mob'):get_transform()
   
   -- no need to set the transform on the scaffolding, since it's just a blueprint
   local scaffolding = radiant.entities.create_entity(uri)
   scaffolding:add_component('stonehearth:construction_data')
                  :set_normal(normal)
   scaffolding:add_component('stonehearth:scaffolding_fabricator'):support_project(project, blueprint, normal)
   radiant.entities.set_faction(scaffolding, project)
   radiant.entities.set_player_id(scaffolding, project)

   -- create a fabricator entity to build the scaffolding
   local name = string.format('[scaffolding for %s]', self.name)
   local fabricator = radiant.entities.create_entity()
   fabricator:set_debug_text('fabricator for scaffolding of ' .. project:get_debug_text())
   fabricator:add_component('mob'):set_transform(transform)
   fabricator:add_component('stonehearth:fabricator')
                              :start_project(name, scaffolding)
                              
   -- render fabricators in blueprint mode
   fabricator:add_component('render_info')
               :set_material('materials/blueprint_gridlines.xml')
               
   -- add the fabricator and the project to our entity container so they get rendered
   self._entity:add_component('entity_container')
                  :add_child(fabricator)

   self._scaffolding_blueprint = scaffolding
   self._scaffolding_fabricator = fabricator
end

return FabricatorComponent
