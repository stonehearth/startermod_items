local Point2  = _radiant.csg.Point2
local Point3  = _radiant.csg.Point3
local Cube3   = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

local INFINITE = 1000000

local log = radiant.log.create_logger('build.scaffolding')
local ScaffoldingManager = class()

function ScaffoldingManager:initialize()
   self._sv.ladder_builders = {}
   self._sv.builders = {}
   self._sv.ladders = {}
   self._sv.scaffolding = {}
   self._sv.fabricators = {}
   self._sv.next_id = 2

   self.__saved_variables:mark_changed()
end

function ScaffoldingManager:activate()
end


function ScaffoldingManager:request_scaffolding_for(requestor, blueprint_rgn, project_rgn, normal)
   checks('self', 'Entity', 'Region3Boxed', 'Region3Boxed', '?Point3')

   local id = self:_get_next_id()
   local builder = radiant.create_controller('stonehearth:build:scaffolding_builder_1d', self, id, requestor, blueprint_rgn, project_rgn, normal)

   self._sv.builders[id] = builder
   self.__saved_variables:mark_changed()

   self:_create_scaffolding_entity(id, requestor, builder, normal)
   
   return builder
end

function ScaffoldingManager:_remove_scaffolding_builder(id)
   if self._sv.ladder_builders[id] then
      self._sv.ladder_builders[id] = nil
      if self._sv.ladders[id] then
         self._sv.ladders[id]:destroy()
         self._sv.ladders[id] = nil
      end
      if self._sv.scaffolding[id] then
         radiant.entities.destroy_entity(self._sv.scaffolding[id])
         self._sv.scaffolding[id] = nil
      end
      if self._sv.fabricators[id] then
         radiant.entities.destroy_entity(self._sv.fabricators[id])
         self._sv.fabricators = nil
      end
   end
end

function ScaffoldingManager:_get_next_id()
   local id = self._sv.next_id 
   self._sv.next_id = self._sv.next_id + 1
   self.__saved_variables:mark_changed()
   return id
end

function ScaffoldingManager:_on_active_changed(id, active)
   checks('self', 'number', 'boolean')
   
   local blueprint = self._sv.scaffolding[id]
   if blueprint then
      stonehearth.build:set_active(blueprint, active)
   end
end

function ScaffoldingManager:_create_scaffolding_entity(id, owner, builder, normal)
   checks('self', 'number', 'Entity', 'controller', '?Point3')

   local scaffolding = radiant.entities.create_entity('stonehearth:scaffolding', { owner = owner })
   if normal then
      scaffolding:add_component('stonehearth:construction_data')
                     :set_normal(normal)
   end

   local rgn = builder:get_scaffolding_region()
   scaffolding:add_component('destination')
                  :set_region(rgn)

   -- create a fabricator entity to build the scaffolding
   local fabricator = radiant.entities.create_entity('', { owner = owner })
   local location = radiant.entities.get_world_grid_location(owner)
   radiant.terrain.place_entity_at_exact_location(fabricator, location)

   fabricator:set_debug_text('(Fabricator for ' .. tostring(scaffolding) .. ')')   
   fabricator:add_component('stonehearth:fabricator')
                              :start_project(scaffolding)

   -- wire up the back pointer so we can find the fab entity from the blueprint
   scaffolding:add_component('stonehearth:construction_progress')   
                 :set_fabricator_entity(fabricator, 'stonehearth:fabricator')

   self._sv.scaffolding[id] = scaffolding
   self._sv.fabricators[id] = fabricator
end


function ScaffoldingManager:_on_scaffolding_region_changed(id)
   local old_ladder = self._sv.ladders[id]
   self._sv.ladders[id] = self:_create_ladder(id)

   if old_ladder then
      old_ladder:destroy()
   end
end

function ScaffoldingManager:_create_ladder(id)
   -- put a ladder in column 0.  this one is 2 units away from the
   -- project in the direction of the normal (as the scaffolding itself
   -- is 1 unit away)
   local builder = self._sv.builders[id]
   local fabricator = self._sv.fabricators[id]
   local region  = builder:get_scaffolding_region()
   local normal  = builder:get_normal()

   local ladder_stencil = Region3(Cube3(Point3(0, -INFINITE, 0) + normal,
                                        Point3(1,  INFINITE, 1) + normal))
   local ladder = region:get():intersect_region(ladder_stencil)
   local height = ladder:get_bounds().max.y - 1
   if height == 0 then
      return      
   end

   local climb_to = radiant.entities.get_world_grid_location(fabricator) + normal + normal
   climb_to.y = climb_to.y + height

   local owner = radiant.entities.get_player_id(self._sv.scaffolding[id])   
   return stonehearth.build:request_ladder_to(owner,
                                              climb_to,
                                              normal)      
end

return ScaffoldingManager
