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

local log = radiant.log.create_logger('scaffolding_builder')
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
   self._sv.builders = {}
   
   local footprint = self:_get_footprint(blueprint_rgn)
   local edges = footprint:get_edge_list()
   for edge in edges:each_edge() do
      -- make a clipping box to pull off just this face of the blueprint
      local min = edge.min.location
      local max = edge.max.location - edge.normal

      local clipper = _radiant.csg.construct_cube3(Point3(min.x, -INFINITE, min.y),
                                                   Point3(max.x,  INFINITE, max.y), 0)

      -- make a new builder..
      local normal = Point3(edge.normal.x, 0, edge.normal.y)
      local builder = manager:_create_builder('stonehearth:build:scaffolding_builder_1d', entity, blueprint_rgn, project_rgn, normal, stand_at_base)
      builder:set_clipper(clipper)

      self._sv.builders[builder] = true
   end
end

function ScaffoldingBuilder_TwoDim:get_scaffolding_region()
   -- 1d does all the work
   return nil
end

function ScaffoldingBuilder_TwoDim:set_active(active)
   for builder, _ in pairs(self._sv.builders) do
      builder:set_active(active)
   end
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
