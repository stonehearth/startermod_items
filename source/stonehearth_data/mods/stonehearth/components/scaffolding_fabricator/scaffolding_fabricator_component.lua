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
   self._listening_to_borrower = {}
   self._sv = self.__saved_variables:get_data()

   radiant.events.listen_once(radiant, 'radiant:game_loaded', function()
         if self._sv.blueprint then
            self:_restore()
         end
      end)
end

function ScaffoldingFabricator:destroy()
   radiant.events.unlisten(self._sv.blueprint, 'stonehearth:construction:teardown_changed', self, self._update_scaffolding_size)
   if self._project_trace then
      self._project_trace:destroy()
      self._project_trace = nil
   end
end

function ScaffoldingFabricator:_restore()
   self._entity_dst = self._entity:get_component('destination')
   self._entity_ladder = self._entity:get_component('vertical_pathing_region')
   self:_start()
end

function ScaffoldingFabricator:support_project(project, blueprint, normal)
   assert(normal)
   
   self._sv.blueprint = blueprint
   self._sv.project = project
   self._sv.normal = normal

   self._entity_dst = self._entity:add_component('destination')
   self._entity_ladder = self._entity:add_component('vertical_pathing_region')
   self._entity_ladder:set_region(_radiant.sim.alloc_region())
                      :set_normal(normal)
   self:_start()
end

function ScaffoldingFabricator:_start()
   -- build a stencil to clip out a ladder from our project.  the ladder
   -- goes in column 0.  the scaffolding is 1 unit away from the project
   -- (in the direction of the normal)
   self._ladder_stencil = Region3(Cube3(Point3(0, -COORD_MAX, 0),
                                        Point3(1,  COORD_MAX, 1)))
   self._ladder_stencil:translate(self._sv.normal)
   
   -- keep track of when the project changes so we can change the scaffolding
   self._project_dst = self._sv.project:get_component('destination')
   self._blueprint_dst = self._sv.blueprint:get_component('destination')
   self._blueprint_cd = self._sv.blueprint:get_component('stonehearth:construction_data')
   self._blueprint_cp = self._sv.blueprint:get_component('stonehearth:construction_progress')
   radiant.events.listen(self._sv.blueprint, 'stonehearth:construction:teardown_changed', self, self._update_scaffolding_size)
   
   self._project_trace = self._project_dst:trace_region('generating scaffolding')
   self._project_trace:on_changed(function()
      self:_update_scaffolding_size()
   end)
   self:_update_scaffolding_size()
end

-- returns whether or not all the blueprints that are borrowing our scafflding
-- have finished.  this whole thing goes away once we implement scaffolding sharing
-- in the build service!
function ScaffoldingFabricator:_borrowers_finished()
   local borrowing_us = self._blueprint_cd:get_loaning_scaffolding_to()

   for id, borrower in pairs(borrowing_us) do
      local borrower_cp = borrower:get_component('stonehearth:construction_progress')
      local finished = borrower_cp:get_finished()
      if not finished then
         -- this one's not finished.  rats.  listen on an event so that when it is
         -- finished, we'll go back and check our scaffolding size.  make sure we don't
         -- listen more than once, though (since that would be extremely wasteful)
         if not self._listening_to_borrower[id] then
            self._listening_to_borrower[id] = true
            radiant.events.listen(borrower, 'stonehearth:construction:finished_changed', function()
                  self._listening_to_borrower[id] = nil
                  self:_update_scaffolding_size()
                  return radiant.events.UNLISTEN
               end)
         end
         -- there's no need to listen on anyone else, even if they're not finished.
         -- one listener is sufficient to start the 'check all borrowers' process all
         -- over again.
         return false
      end
   end
   return true
end

function ScaffoldingFabricator:_update_scaffolding_size()
   local blueprint_teardown = self._blueprint_cp:get_teardown()
   local blueprint_finished = self._blueprint_cp:get_finished()
   if not blueprint_teardown then
      if blueprint_finished then
         if self:_borrowers_finished() then
            -- make our region completely empty.  the fabricator will worry about
            -- the exact details of how this happens
            self:_clear_destination_region()
         end
      else
         self:_cover_project_region()
      end
   else
      self:_cover_project_region()
      -- now strip off the top row...
      self._entity_dst:get_region():modify(function(cursor) 
         local bounds = cursor:get_bounds()
         if cursor:get_area() > 0 then
            local top_row = Cube3(Point3(-COORD_MAX, bounds.max.y - 1, -COORD_MAX),
                                  Point3( COORD_MAX, bounds.max.y,  COORD_MAX))
            cursor:subtract_cube(top_row)
         end
      end)

   end
   self:_update_ladder_region()
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
                               
         if not (blueprint_rgn:clipped(clipper) - project_rgn):empty() then
            bounds.max = bounds.max - Point3(0, 1, 0)
         end

         -- create a vertical stripe in the scaffolding reaching from
         -- the top of the computed project region all the way down to the
         -- base of the scaffolding.  this is to ensure that the scaffolding
         -- is solid (e.g. obscures doors and windows), without covering parts
         -- of walls that will be occupied by roof (e.g. the angled supporters
         -- of a peaked roof)

         -- first make sure the scaffolding fills in all the holes across gaps
         -- (consider the first row of a wall with a door in it)
         local top_row_clipper = Cube3(Point3(bounds.min.x, bounds.max.y - 1, bounds.min.z), bounds.max)
         local top_row_bounds = project_rgn:clipped(top_row_clipper):get_bounds()
         if top_row_bounds then
            local column = Cube3(Point3(top_row_bounds.min.x, 0, top_row_bounds.min.z), top_row_bounds.max)
            column:translate(self._sv.normal)
            cursor:add_cube(column)
         end

         -- now make sure everything goes all the way down to the bottom
         for cube in project_rgn:each_cube() do
            local y = math.min(bounds.max.y, cube.max.y)
            local column = Cube3(Point3(cube.min.x, 0, cube.min.z),
                                 Point3(cube.max.x, y, cube.max.z))
            column:translate(self._sv.normal)
            cursor:add_cube(column)
         end

         -- finally, don't put scaffolding where the project is slated to go!
         cursor:subtract_region(project_rgn)
      end
   end)
end

function ScaffoldingFabricator:_update_ladder_region()
   -- put a ladder in column 0.  this one is 2 units away from the
   -- project in the direction of the normal (as the scaffolding itself
   -- is 1 unit away)
   local region = self._entity_dst:get_region():get()
   local ladder = _radiant.csg.intersect_region3(region, self._ladder_stencil)
   ladder:translate(self._sv.normal)
   self._entity_ladder:get_region():modify(function(cursor)
      cursor:copy_region(ladder)
   end)
   if not self._entity_ladder:get_region():get():empty() then
      log:debug('grew the ladder!')
   end
   log:debug('(%s) updating scaffolding supporting %s -> %s (ladder:%s)', tostring(self._entity), self._sv.project, region, tostring(ladder:get_bounds()))
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

