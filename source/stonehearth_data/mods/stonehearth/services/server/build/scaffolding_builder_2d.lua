local priorities = require('constants').priorities.simple_labor
local build_util = require 'stonehearth.lib.build_util'

local Rect2   = _radiant.csg.Rect2
local Cube3   = _radiant.csg.Cube3
local Point2  = _radiant.csg.Point2
local Point3  = _radiant.csg.Point3
local Region2 = _radiant.csg.Region2
local Region3 = _radiant.csg.Region3
local Array2D = _radiant.csg.Array2D
local TraceCategories = _radiant.dm.TraceCategories

local INFINITE = 1000000

local ScaffoldingBuilder_TwoDim = class()

ScaffoldingBuilder_TwoDim.SOLID_STRUCTURE_TYPES = {
   'stonehearth:roof',
   'stonehearth:wall',
   'stonehearth:column',
   'stonehearth:floor',
}

ScaffoldingBuilder_TwoDim.BUILD = 'build'
ScaffoldingBuilder_TwoDim.TEAR_DOWN = 'teardown'

function ScaffoldingBuilder_TwoDim:initialize(manager, id, entity, blueprint_rgn, project_rgn, normal, stand_at_base)
   checks('self', 'controller', 'number', 'Entity', 'Region3Boxed', 'Region3Boxed', '?Point3', 'boolean')

   self._sv.id = id
   self._sv.teardown = false
   self._sv.manager = manager
   self._sv.entity = entity
   self._sv.blueprint_rgn = blueprint_rgn
   self._sv.project_rgn = project_rgn
   self._sv.stand_at_base = stand_at_base
   self._sv.builders = {}
end

function ScaffoldingBuilder_TwoDim:destroy()
   self._log:info('destroying scaffolding builder')
   for builder, _ in pairs(self._sv.builders) do
      builder:destroy()
   end
   self._sv.builders = {}
end

function ScaffoldingBuilder_TwoDim:activate()
   local entity = self._sv.entity
   
   self._debug_text = ''
   local name = radiant.entities.get_display_name(entity)
   if name then
      self._debug_text = self._debug_text .. ' ' .. name
   else
      self._debug_text = self._debug_text .. ' ' .. entity:get_uri()
   end
   local debug_text = entity:get_debug_text()
   if debug_text then
      self._debug_text = self._debug_text .. ' ' .. debug_text
   end

   self._log = radiant.log.create_logger('build.scaffolding.2d')
                              :set_entity(entity)
                              :set_prefix('s2d:' .. tostring(self._sv.id) .. ' ' .. self._debug_text)

   radiant.events.listen(self._sv.entity, 'radiant:entity:pre_destroy', function()
         self._sv.manager:_remove_builder(self._sv.id)
      end)                              
end

function  ScaffoldingBuilder_TwoDim:set_teardown(teardown)
   self._sv.teardown = teardown
   self.__saved_variables:mark_changed()

   for builder, _ in pairs(self._sv.builders) do
      builder:set_teardown(teardown)
   end
end

function  ScaffoldingBuilder_TwoDim:set_active(active)
   checks('self', '?boolean')

   if active ~= self._sv.active then
      self._log:detail('active changed to %s', active)
      self._sv.active = active
      self.__saved_variables:mark_changed()
      self:_update_status()
   end
end

function ScaffoldingBuilder_TwoDim:_update_status()
   local active = self._sv.active

   if radiant.empty(self._sv.builders) then
      self:_create_builders()
   end
   for builder, _ in pairs(self._sv.builders) do
      builder:set_teardown(self._sv.teardown)
      builder:set_active(active)
   end
end

function ScaffoldingBuilder_TwoDim:_create_builders()
   local footprint = self:_get_footprint(self._sv.blueprint_rgn)
   local edges = footprint:get_edge_list()
   
   for edge in edges:each_edge() do
      -- make a clipping box to pull off just this face of the blueprint
      local min = edge.min.location
      local max = edge.max.location - edge.normal

      local clipper = _radiant.csg.construct_cube3(Point3(min.x, -INFINITE, min.y),
                                                   Point3(max.x,  INFINITE, max.y), 0)

      -- make a new builder..
      local normal = Point3(edge.normal.x, 0, edge.normal.y)

      self._log:detail('creating 1d builder with clip box %s and normal %s', clipper, normal)

      local builder = self._sv.manager:_create_builder('stonehearth:build:scaffolding_builder_1d',
                                              self._sv.entity,
                                              self._sv.blueprint_rgn,
                                              self._sv.project_rgn,
                                              normal,
                                              self._sv.stand_at_base)
      builder:set_clipper(clipper)

      self._sv.builders[builder] = true
   end
end

function ScaffoldingBuilder_TwoDim:get_scaffolding_region()
   -- 1d does all the work
   return nil
end

function ScaffoldingBuilder_TwoDim:_get_footprint(blueprint_rgn)
   -- calculate the local footprint of the floor.
   local footprint = Region2()
   for cube in blueprint_rgn:get():each_cube() do
      local rect = Rect2(Point2(cube.min.x, cube.min.z),
                         Point2(cube.max.x, cube.max.z))
      footprint:add_cube(rect)
   end
   return footprint
end

return ScaffoldingBuilder_TwoDim
