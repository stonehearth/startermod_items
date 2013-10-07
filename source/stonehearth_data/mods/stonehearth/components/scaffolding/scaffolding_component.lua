local Scaffolding = class()
local Point2 = _radiant.csg.Point2
local Region3 = _radiant.csg.Region3
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3

local COORD_MAX = 1000000 -- 1 million enough?
local LADDER_STENCIL = Region3(Cube3(Point3(0, -COORD_MAX, 0),
                                     Point3(1,  COORD_MAX, 1)))

-- this is the component which manages the Scaffolding entity.
-- this is the blueprint for the scaffolding.  the actual scaffolding
-- which appears in the world is created by a fabricator.
function Scaffolding:__init(entity, data_binding)
   self._entity = entity
end

function Scaffolding:support_project(project, normal)
   self._project = project
   self._normal = normal
   self._entity_dst = self._entity:add_component('destination')
   self._entity_ladder = self._entity:add_component('vertical_pathing_region')
   self._entity_ladder:set_region(_radiant.sim.alloc_region())
                      :set_normal(normal)
   
   -- build a stencil to clip out a ladder from our project.  the ladder
   -- goes in column 0.  the scaffolding is 1 unit away from the project
   -- (in the direction of the normal)
   self._ladder_stencil = LADDER_STENCIL
   self._ladder_stencil:translate(normal)
   
   -- keep track of when the project changes so we can change the scaffolding
   self._project_dst = self._project:get_component('destination')
   
   self._project_trace = self._project_dst:trace_region('generating scaffolding')
   self._project_trace:on_changed(function()
      self:_update_scaffolding_size()
   end)
end

function Scaffolding:get_scaffolding()
   return self._scaffolding
end

function Scaffolding:_update_scaffolding_size()
   local project_rgn = self._project_dst:get_region():get()
   
   -- scaffolding is 1 unit away from and not overlapping the project.
   local cursor = self._entity_dst:get_region():modify()
   cursor:copy_region(project_rgn)
   cursor:translate(self._normal)
   cursor:subtract_region(project_rgn)
   
   -- put a ladder in column 0.  this one is 2 units away from the
   -- project in the direction of the normal (as the scaffolding itself
   -- is 1 unit away)
   local ladder = _radiant.csg.region3_intersection(cursor, self._ladder_stencil)
   ladder:translate(self._normal)
   self._entity_ladder:get_region():modify():copy_region(ladder)
   
   radiant.log.info('(%s) updating scaffolding supporting %s -> %s (ladder:%s)', tostring(self._entity), self._project, cursor, tostring(ladder:get_bounds()))
end

return Scaffolding
