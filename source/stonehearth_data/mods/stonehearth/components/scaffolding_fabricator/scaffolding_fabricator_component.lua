local ScaffoldingFabricator = class()
local Point2 = _radiant.csg.Point2
local Region3 = _radiant.csg.Region3
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local log = radiant.log.create_logger('build')

local COORD_MAX = 1000000 -- 1 million enough?

-- this is the component which manages the ScaffoldingFabricator entity.
-- this is the blueprint for the scaffolding.  the actual scaffolding
-- which appears in the world is created by a fabricator.
function ScaffoldingFabricator:initialize(entity, json)
   self._entity = entity
end

function ScaffoldingFabricator:support_project(project, blueprint, normal)
   assert(normal)
   
   self._blueprint = blueprint
   self._project = project
   self._normal = normal
   self._entity_dst = self._entity:add_component('destination')
   self._entity_ladder = self._entity:add_component('vertical_pathing_region')
   self._entity_ladder:set_region(_radiant.sim.alloc_region())
                      :set_normal(normal)
   
   -- build a stencil to clip out a ladder from our project.  the ladder
   -- goes in column 0.  the scaffolding is 1 unit away from the project
   -- (in the direction of the normal)
   self._ladder_stencil = Region3(Cube3(Point3(0, -COORD_MAX, 0),
                                        Point3(1,  COORD_MAX, 1)))
   self._ladder_stencil:translate(normal)
   
   -- keep track of when the project changes so we can change the scaffolding
   self._project_dst = self._project:get_component('destination')
   self._blueprint_dst = self._blueprint:get_component('destination')
   
   self._project_trace = self._project_dst:trace_region('generating scaffolding')
   self._project_trace:on_changed(function()
      self:_update_scaffolding_size()
   end)
end

function ScaffoldingFabricator:get_scaffolding()
   return self._scaffolding
end

function ScaffoldingFabricator:_update_scaffolding_size()
   local finished = self._project:add_component('stonehearth:construction_progress')
                                    :get_finished()
   if not finished then
      self:_cover_project_region()
   else
      -- make our region completely empty.  the fabricator will worry about
      -- the exact details of how this happens
      self:_clear_destination_region()
   end
end

function ScaffoldingFabricator:_cover_project_region()
   local project_rgn = self._project_dst:get_region():get()
   local blueprint_rgn = self._blueprint_dst:get_region():get()
   
   self._entity_dst:get_region():modify(function(cursor)
      -- scaffolding is 1 unit away from the project.  this is a huge
      -- optimization to make sure the scaffolding both reaches the ground
      -- at every point and has no gaps in it when the project has gaps
      -- (e.g. for doors and windows).  it will not work, however, for
      -- irregularaly sized projects.  revisit if those ever show up!
      local bounds = project_rgn:get_bounds()
      cursor:clear()
      
      if bounds:get_area() > 0 then
         -- if the top row isn't finished, lower the height by 1.  the top
         -- row is finished if the project - the blueprint is empty
         local clipper = Cube3(Point3(-COORD_MAX, bounds.min.y, -COORD_MAX),
                               Point3( COORD_MAX, bounds.max.y,  COORD_MAX))
                               
         if not (blueprint_rgn:clip(clipper) - project_rgn):empty() then
            bounds.max = bounds.max - Point3(0, 1, 0)
         end
         if bounds:get_area() > 0 then
            local rgn = Region3()
            rgn:add_cube(bounds:translated(self._normal))

            -- finally, don't overlap the project
            cursor:copy_region(rgn - project_rgn)
         end
      end
   end)
   
   -- put a ladder in column 0.  this one is 2 units away from the
   -- project in the direction of the normal (as the scaffolding itself
   -- is 1 unit away)
   local region = self._entity_dst:get_region():get()
   local ladder = _radiant.csg.intersect_region3(region, self._ladder_stencil)
   ladder:translate(self._normal)
   self._entity_ladder:get_region():modify(function(cursor)
      cursor:copy_region(ladder)
   end)
   if not self._entity_ladder:get_region():get():empty() then
      log:debug('grew the ladder!')
   end
   log:debug('(%s) updating scaffolding supporting %s -> %s (ladder:%s)', tostring(self._entity), self._project, region, tostring(ladder:get_bounds()))
end

function ScaffoldingFabricator:_clear_destination_region()
   self._entity_dst:get_region():modify(function(cursor)
      cursor:clear()
   end)
   self._entity_ladder:get_region():modify(function(cursor)
      cursor:clear()
   end)
end

return ScaffoldingFabricator

