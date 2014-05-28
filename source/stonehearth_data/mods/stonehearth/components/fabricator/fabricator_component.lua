local Fabricator = require 'components.fabricator.fabricator'
local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local Cube3 = _radiant.csg.Cube3
local Color4 = _radiant.csg.Color4
local log = radiant.log.create_logger('commands')

local FabricatorComponent = class()

-- this is the component which manages the fabricator entity.
function FabricatorComponent:initialize(entity, json)
   self._log = radiant.log.create_logger('build')
                        :set_prefix('fab component')
                        :set_entity(entity)

   self._entity = entity
   self._sv = self.__saved_variables:get_data()


   if not self._sv.initialized then
      self._sv.initialized = true
      self._sv.active = false
      self._sv.teardown = false
      self.__saved_variables:mark_changed()
   end

   radiant.events.listen_once(radiant, 'radiant:game_loaded', function()
         if self._sv.blueprint and self._sv.project then
            self._fabricator = Fabricator(string.format("(%s Fabricator)", tostring(self._sv.blueprint)),
                                          self._entity,
                                          self._sv.blueprint,
                                          self._sv.project)
            self._fabricator:set_teardown(self._sv.teardown)
            self._fabricator:set_active(self._sv.active)
            radiant.events.listen_once(self._sv.blueprint, 'radiant:entity:pre_destroy', self, self._on_blueprint_destroyed)
         end
      end)
end

function FabricatorComponent:destroy()
   self._log:debug('destroying fabricator component')
   
   -- destroy the fabricator.  this will also destroy the project.
   if self._fabricator then
      self._fabricator:destroy()
      self._fabricator = nil
   end

   -- destroy all the scaffolding stuff we created.
   if self._sv.scaffolding_blueprint then
      radiant.entities.destroy_entity(self._sv.scaffolding_blueprint)
      self._sv.scaffolding_blueprint = nil
   end
   if self._sv.scaffolding_fabricator then
      radiant.entities.destroy_entity(self._sv.scaffolding_fabricator)
      self._sv.scaffolding_fabricator = nil
   end
end

function FabricatorComponent:set_active(enabled)
   self._log:info('setting active to %s', tostring(enabled))
   
   self._fabricator:set_active(enabled)
   
   -- scaffolding entities are child of the fabricator, so start them, too
   if self._sv.scaffolding_blueprint then
      stonehearth.build:set_active(self._sv.scaffolding_blueprint, enabled)
   end

   self._sv.active = enabled
   self.__saved_variables:mark_changed()
end

function FabricatorComponent:set_teardown(enabled)
   self._log:info('setting teardown to %s', tostring(enabled))
   
   self._fabricator:set_teardown(enabled)
   self._sv.teardown = enabled
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

function FabricatorComponent:start_project(blueprint)
   self._log:debug('starting project for %s', blueprint)
   
   self._fabricator = Fabricator(string.format("(%s Fabricator)", tostring(blueprint)),
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
   
   radiant.events.listen_once(self._sv.blueprint, 'radiant:entity:pre_destroy', self, self._on_blueprint_destroyed)

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
   local fabricator = radiant.entities.create_entity()
   fabricator:set_debug_text('(Fabricator for ' .. tostring(scaffolding) .. ')')   
   fabricator:add_component('mob'):set_transform(transform)
   fabricator:add_component('stonehearth:fabricator')
                              :start_project(scaffolding)
                              
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

-- called just before the blueprint for this fabricator is destroyed.  we need only
-- destroy the entity for this fabricator.  the rest happens directly as a side-effect
-- of doing so (see :destroy())
function FabricatorComponent:_on_blueprint_destroyed()
   if self._entity then
      radiant.entities.destroy_entity(self._entity)
   end
end

return FabricatorComponent
