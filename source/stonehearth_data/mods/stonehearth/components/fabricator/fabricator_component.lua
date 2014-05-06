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
   self._sv = self.__saved_variables:get_data()

   if not self._sv.initialized then
      self._sv.initialized = true
      self._sv.active = false
      self.__saved_variables:mark_changed()
   end
   radiant.events.listen_once(radiant, 'radiant:game_loaded', function()
         if self._sv.blueprint and self._sv.project then
            self._fabricator = Fabricator(string.format("(%s Fabricator)", tostring(self._sv.blueprint)),
                                          self._entity,
                                          self._sv.blueprint,
                                          self._sv.project)
            self._fabricator:set_active(self._sv.active)
         end
      end)
end

function FabricatorComponent:destroy()
   log:debug('destroying fabricator component for %s', self._entity)
   
   if self._fabricator then
      self._fabricator:destroy()
      self._fabricator = nil
   end
   if self._sv.scaffolding_blueprint then
      radiant.entities.destroy_entity(self._sv.scaffolding_blueprint)
      self._sv.scaffolding_blueprint = nil
   end
   if self._sv.scaffolding_fabricator then
      radiant.entities.destroy_entity(self._sv.scaffolding_fabricator)
      self._sv.scaffolding_fabricator = nil
   end
end

function FabricatorComponent:set_active(active)
   log:error('%s setting active to %s', self._entity, tostring(active))
   
   self._fabricator:set_active(active)
   
   -- scaffolding entities are child of the fabricator, so start them, too
   if self._sv.scaffolding_blueprint then
      stonehearth.build:set_building_active(self._sv.scaffolding_blueprint, active)
   end

   self._sv.active = active
   self.__saved_variables:mark_changed()
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
   self._sv.name = name and name or '-- unnamed --'
   log:debug('starting project %s', self._sv.name)
   
   self._fabricator = Fabricator(string.format("(%s Fabricator)", tostring(self._sv.blueprint)),
                                 self._entity,
                                 blueprint)
   
   -- finally, put it on the ground.  we also need to put ourselves on the ground
   -- so the pathfinder can find it's way to regions which need to be constructed
   local project = self._fabricator:get_project()

   local ci = blueprint:get_component('stonehearth:construction_data')
   if ci:needs_scaffolding() then
      local normal = ci:get_normal()
      assert(radiant.util.is_a(normal, Point3))
      self:_add_scaffolding(blueprint, project, normal)
   end

   -- remember the blueprint and project
   self._sv.project = project
   self._sv.blueprint = blueprint
   self.__saved_variables:mark_changed()
   
   return self
end

function FabricatorComponent:get_blueprint()
   return self._sv.blueprint
end

function FabricatorComponent:get_project()
   return self._sv.project
end

function FabricatorComponent:_add_scaffolding(blueprint, project, normal)
   -- create a scaffolding blueprint and point it to the project
   local uri = 'stonehearth:scaffolding'
   local transform = project:add_component('mob'):get_transform()
   
   -- ask the build service to set all this up!!

   local scaffolding = radiant.entities.create_entity('stonehearth:scaffolding')
   radiant.entities.set_faction(scaffolding, project)
   radiant.entities.set_player_id(scaffolding, project)
   scaffolding:add_component('stonehearth:construction_data')
                  :set_normal(normal)

   -- no need to set the transform on the scaffolding, since it's just a blueprint
   scaffolding:add_component('stonehearth:scaffolding_fabricator')
                  :support_project(project, blueprint, normal)

   -- create a fabricator entity to build the scaffolding
   local name = string.format('[scaffolding for %s]', tostring(blueprint))
   local fabricator = radiant.entities.create_entity()
   fabricator:set_debug_text('(Fabricator for ' .. tostring(scaffolding) .. ')')   
   fabricator:add_component('mob'):set_transform(transform)
   fabricator:add_component('stonehearth:fabricator')
                              :start_project(name, scaffolding)
                              
   -- add the fabricator and the project to our entity container so they get rendered
   self._entity:add_component('entity_container')
                  :add_child(fabricator)

   -- wire up the back pointer so we can find the fab entity from the blueprint
   scaffolding:add_component('stonehearth:construction_progress')   
               :set_fabricator_entity(fabricator)

   self._sv.scaffolding_blueprint = scaffolding
   self._sv.scaffolding_fabricator = fabricator
   self.__saved_variables:mark_changed()
end

return FabricatorComponent
