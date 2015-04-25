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
            self:_restore()
         end
      end)
end

function FabricatorComponent:_restore()
   self._fabricator = Fabricator(string.format("(%s Fabricator)", tostring(self._sv.blueprint)),
                                 self._entity,
                                 self._sv.blueprint,
                                 self._sv.project,
                                 self._sv.total_mining_region)
   self._fabricator:set_scaffolding(self._sv.scaffolding)
   self._fabricator:set_teardown(self._sv.teardown)
   self._fabricator:set_active(self._sv.active)
   radiant.events.listen_once(self._sv.blueprint, 'radiant:entity:pre_destroy', self, self._destroy_self)
   radiant.events.listen_once(self._sv.project,   'radiant:entity:pre_destroy', self, self._destroy_self)
end

function FabricatorComponent:destroy()
   self._log:debug('destroying fabricator component')
   
   -- destroy the fabricator.  this will also destroy the project.
   if self._fabricator then
      self._fabricator:destroy()
      self._fabricator = nil
   end

   -- destroy all the scaffolding stuff we created.
   if self._sv.scaffolding then
      self._sv.scaffolding:destroy()
      self._sv.scaffolding = nil
   end
end

function FabricatorComponent:instabuild()
   self._fabricator:instabuild()
end

function FabricatorComponent:set_active(enabled)
   self._log:info('setting active to %s', tostring(enabled))
   
   self._fabricator:set_active(enabled)
   
   self._sv.active = enabled
   self.__saved_variables:mark_changed()
end

function FabricatorComponent:set_teardown(enabled)
   self._log:info('setting teardown to %s', tostring(enabled))
   
   self._sv.teardown = enabled
   self._fabricator:set_teardown(enabled)
   self.__saved_variables:mark_changed()  
end

function FabricatorComponent:start_project(blueprint)
   self._log:debug('starting project for %s', blueprint)
   
   self._fabricator = Fabricator(string.format("(%s Fabricator)", tostring(blueprint)),
                                 self._entity,
                                 blueprint)
   
   -- finally, put it on the ground.  we also need to put ourselves on the ground
   -- so the pathfinder can find it's way to regions which need to be constructed
   local project = self._fabricator:get_project()

   -- add scaffolding!
   local ci = blueprint:get_component('stonehearth:construction_data')
   local normal = ci:get_normal()
   local project_rgn = project:get_component('destination'):get_region()
   local blueprint_rgn = blueprint:get_component('destination'):get_region()
   local stand_at_base = ci:get_project_adjacent_to_base()
   if blueprint:get_uri() ~= 'stonehearth:build:prototypes:scaffolding' then
      local scaffolding = stonehearth.build:request_scaffolding_for(self._entity, blueprint_rgn, project_rgn, normal, stand_at_base)
      self._fabricator:set_scaffolding(scaffolding)
      self._sv.scaffolding = scaffolding
   end
   
   -- remember the blueprint and project
   self._sv.project = project
   self._sv.blueprint = blueprint
   self._sv.total_mining_region = self._fabricator:get_total_mining_region()
   self.__saved_variables:mark_changed()
   
   radiant.events.listen_once(self._sv.blueprint, 'radiant:entity:pre_destroy', self, self._destroy_self)
   radiant.events.listen_once(self._sv.project,   'radiant:entity:pre_destroy', self, self._destroy_self)

   return self
end

function FabricatorComponent:get_blueprint()
   return self._sv.blueprint
end

function FabricatorComponent:get_project()
   return self._sv.project
end

-- called just before the blueprint for this fabricator is destroyed.  we need only
-- destroy the entity for this fabricator.  the rest happens directly as a side-effect
-- of doing so (see :destroy())
function FabricatorComponent:_destroy_self()
   if self._entity and self._entity:is_valid() then
      radiant.entities.destroy_entity(self._entity)
   end
end

function FabricatorComponent:accumulate_costs(cost)
   local blueprint = self._sv.blueprint
   local material = blueprint:get_component('stonehearth:construction_data')
                                    :get_material()

   local rgn = blueprint:get_component('destination')
                           :get_region()
                              :get()
   
   for cube in rgn:each_cube() do
      local area = cube:get_area()
      local world_point = radiant.entities.local_to_world(cube.min, self._entity)
      local material = self._fabricator:get_material(world_point)
      cost.resources[material] = (cost.resources[material] or 0) + area
   end
end

return FabricatorComponent
