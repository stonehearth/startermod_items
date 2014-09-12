local ScaffoldingFabricator = class()
local Point2 = _radiant.csg.Point2
local Region3 = _radiant.csg.Region3
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Point3 = _radiant.csg.Point3
local Array2D = _radiant.csg.Array2D
local TraceCategories = _radiant.dm.TraceCategories

local COORD_MAX = 1000000 -- 1 million enough?
local log = radiant.log.create_logger('build')

-- this is the component which manages the ScaffoldingFabricator entity.
-- this is the blueprint for the scaffolding.  the actual scaffolding
-- which appears in the world is created by a fabricator.
function ScaffoldingFabricator:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()

   radiant.events.listen_once(radiant, 'radiant:game_loaded', function()
         if self._sv.blueprint and self._sv.project then
            self:_restore()
         end
      end)
end

function ScaffoldingFabricator:destroy()
   self._teardown_listener:destroy()
   self._teardown_listener = nil

   self._finished_listener:destroy()
   self._finished_listener = nil

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
   self._entity_ladder:set_region(_radiant.sim.alloc_region3())
                      :set_normal(normal)
   self.__saved_variables:mark_changed()
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
   self._teardown_listener = radiant.events.listen(self._sv.blueprint, 'stonehearth:construction:teardown_changed', self, self._update_scaffolding_size)
   self._finished_listener = radiant.events.listen(self._sv.blueprint, 'stonehearth:construction:finished_changed', self, self._update_scaffolding_size)
   
   self._project_trace = self._project_dst:trace_region('generating scaffolding', TraceCategories.SYNC_TRACE)
                                             :on_changed(function()
                                                   self:_update_scaffolding_size()
                                                end)
                                             :push_object_state()
end

-- returns whether or not all the blueprints that are borrowing our scafflding
-- have finished.  this whole thing goes away once we implement scaffolding sharing
-- in the build service!
function ScaffoldingFabricator:_compute_borrowers_required_height()
   local desired_height
   local borrowing_us = self._blueprint_cd:get_loaning_scaffolding_to()
   for id, borrower in pairs(borrowing_us) do
      local borrower_cp = borrower:get_component('stonehearth:construction_progress')
      local finished = borrower_cp:get_finished()
      if not finished then
         -- this one's not finished.  rats.  listen on an event so that when it is
         -- finished, we'll go back and check our scaffolding size.
         radiant.events.listen_once(borrower, 'stonehearth:construction:finished_changed', function()
               self:_update_scaffolding_size()
            end)

         -- make sure we build up at least to the base of whatever it is that's
         -- borrowing us
         local borrower_fabricator = borrower_cp:get_fabricator_entity()
         if borrower_fabricator then
            -- assumes the scaffolding and the fixture are in the same coordinate sytem, which
            -- should be fine since the fixture is a child of the thing we're providing scaffolding
            -- for!
            local bottom = borrower_fabricator:get_component('mob'):get_grid_location()
            local dst = borrower_fabricator:get_component('destination')
            if dst then
               bottom = bottom + dst:get_region():get():get_bounds().min
            end
            desired_height = math.max(desired_height and desired_height or 0, bottom.y)
         end
      end
   end
   return desired_height
end

function ScaffoldingFabricator:_update_scaffolding_size()
   local blueprint_teardown = self._blueprint_cp:get_teardown()
   local blueprint_finished = self._blueprint_cp:get_finished()
   if not blueprint_teardown then
      if not blueprint_finished then
         -- still not done with the blueprint.  cover the whole thing
         self:_cover_project_region()
      else
         local desired_height = self:_compute_borrowers_required_height()
         if desired_height then
            -- borrowers still need us up to `desired_height`.  build
            -- down to there
            self:_cover_project_region(desired_height)
         else
            -- completely done!  erase our region
            self:_clear_destination_region()
         end
      end
   else
      self:_cover_project_region()
   end
   self:_update_ladder_region()
end

function ScaffoldingFabricator:_cover_project_region(max_height)
   local teardown = self._blueprint_cp:get_teardown()
   local project_rgn = self._project_dst:get_region():get()
   local blueprint_rgn = self._blueprint_dst:get_region():get()
   
   self._entity_dst:get_region():modify(function(cursor)
      -- scaffolding is 1 unit away from the project.  this is a huge
      -- optimization to make sure the scaffolding both reaches the ground
      -- at every point and has no gaps in it when the project has gaps
      -- (e.g. for doors and windows).  it will not work, however, for
      -- irregularaly sized projects.  revisit if those ever show up!
      local project_bounds = project_rgn:get_bounds()
      cursor:clear()
      
      if project_bounds:get_area() > 0 then
         local blueprint_bounds = blueprint_rgn:get_bounds()
         local is_grounded = blueprint_bounds.min.y == 0

         local size = blueprint_bounds:get_size()
         local origin = Point2(blueprint_bounds.min.x, blueprint_bounds.min.z)

         -- compute the max height for every column in the blueprint
         local blueprint_min_heights
         local blueprint_max_heights = Array2D(size.x, size.z, origin, -COORD_MAX)
         if not is_grounded then
            blueprint_min_heights = Array2D(size.x, size.z, origin, COORD_MAX)
         end

         for cube in blueprint_rgn:each_cube() do
            for x = cube.min.x, cube.max.x - 1 do
               for z = cube.min.z, cube.max.z - 1 do
                  local h = blueprint_max_heights:get(x, z)
                  blueprint_max_heights:set(x, z, math.max(h, cube.max.y - 1))
                  if not is_grounded then
                     h = blueprint_min_heights:get(x, z)
                     blueprint_min_heights:set(x, z, math.min(h, cube.min.y))
                  end
               end
            end
         end

         local subtract_top_row = false
         if teardown then
            -- if we're tearing down, always build 1 below the top row
            subtract_top_row = true
         else
            -- if the top row isn't finished, lower the height by 1.  the top
            -- row is finished if the project - the blueprint is empty
            local clipper = Cube3(Point3(-COORD_MAX, project_bounds.min.y, -COORD_MAX),
                                  Point3( COORD_MAX, project_bounds.max.y,  COORD_MAX))

            if not (blueprint_rgn:clipped(clipper) - project_rgn):empty() then
               subtract_top_row = true
            end
         end

         if subtract_top_row then
            project_bounds.max = project_bounds.max - Point3.unit_y
         end

         if max_height then
            max_height = math.min(max_height, project_bounds.max.y)
         else
            max_height = project_bounds.max.y
         end
   
         -- add a piece of scaffolding for every xz column in `bounds`.  the
         -- height of the scaffolding will be the min of the max height and the
         -- previously computed blueprint_max_heights entry for that xz position
         local scaffolding_heights = Array2D(size.x, size.z, origin, -COORD_MAX)               
         local function add_scaffolding(bounds)
            local xz_bounds = Cube3(Point3(bounds.min.x, 0, bounds.min.z),
                                    Point3(bounds.max.x, 1, bounds.max.z))
                                              
            for pt in xz_bounds:each_point() do
               local h = blueprint_max_heights:get(pt.x, pt.z)
               h = math.min(h, max_height)
               scaffolding_heights:set(pt.x, pt.z, h)
             end
         end

         -- create a vertical stripe in the scaffolding reaching from
         -- the top of the computed project region all the way down to the
         -- base of the scaffolding.  this is to ensure that the scaffolding
         -- is solid (e.g. obscures doors and windows), without covering parts
         -- of walls that will be occupied by roof (e.g. the angled supporters
         -- of a peaked roof)

         -- first make sure the scaffolding fills in all the holes across gaps
         -- (consider the first row of a wall with a door in it)
         if max_height > 0 then
            local top_row_clipper = Cube3(Point3(-COORD_MAX, project_bounds.max.y, -COORD_MAX),
                                          Point3( COORD_MAX, COORD_MAX,  COORD_MAX))
            local blueprint_top_row = blueprint_rgn:clipped(top_row_clipper)
            add_scaffolding(blueprint_top_row:get_bounds())

            -- now make sure everything completely covers the project
            add_scaffolding(project_rgn:get_bounds())
         end

         -- finally, iterate through every column in the scaffolding heights
         -- array and add the columns        
         for x = blueprint_bounds.min.x, blueprint_bounds.max.x - 1 do
            for z = blueprint_bounds.min.z, blueprint_bounds.max.z - 1 do                  
               local base, h = 0, scaffolding_heights:get(x, z)
               if h ~= -COORD_MAX then
                  if not is_grounded then
                     -- if the region is floating in the air, we put the base of the
                     -- column at the base of the blueprint
                     assert(blueprint_min_heights)
                     base = blueprint_min_heights:get(x, z)
                     assert(base ~= COORD_MAX)
                  end
                  if base < h then
                     local column = Cube3(Point3(x, base, z), 
                                          Point3(x + 1, h, z + 1))
                     column:translate(self._sv.normal)
                     cursor:add_unique_cube(column)
                  end
               end
            end
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
   log:debug('(%s) updating scaffolding supporting %s -> %s (ladder:%s)', tostring(self._entity), self._sv.project, region:get_bounds(), tostring(ladder:get_bounds()))
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

