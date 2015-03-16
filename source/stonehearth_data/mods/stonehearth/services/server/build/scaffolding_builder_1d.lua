local priorities = require('constants').priorities.simple_labor
local build_util = require 'stonehearth.lib.build_util'

local Cube3   = _radiant.csg.Cube3
local Point2  = _radiant.csg.Point2
local Point3  = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local Array2D = _radiant.csg.Array2D
local TraceCategories = _radiant.dm.TraceCategories

local log = radiant.log.create_logger('scaffolding_builder')
local INFINITE = 1000000

local ScaffoldingBuilder_OneDim = class()

ScaffoldingBuilder_OneDim.SOLID_STRUCTURE_TYPES = {
   'stonehearth:roof',
   'stonehearth:wall',
   'stonehearth:column',
   'stonehearth:floor',
}

ScaffoldingBuilder_OneDim.BUILD = 'build'
ScaffoldingBuilder_OneDim.TEAR_DOWN = 'teardown'

function ScaffoldingBuilder_OneDim:initialize(manager, id, entity, blueprint_rgn, project_rgn, normal)
   checks('self', 'controller', 'number', 'Entity', 'Region3Boxed', 'Region3Boxed', 'Point3')

   self._sv.id = id
   self._sv.active = false
   self._sv.entity = entity
   self._sv.manager = manager
   self._sv.normal = normal
   self._sv.project_rgn = project_rgn
   self._sv.blueprint_rgn = blueprint_rgn
   self._sv.mode = ScaffoldingBuilder_OneDim.BUILD 
   self._sv.scaffolding_rgn = _radiant.sim.alloc_region3()   
end

function ScaffoldingBuilder_OneDim:activate()
   radiant.events.listen(self._sv.entity, 'radiant:entity:pre_destroy', function()
         self._sv.manager:_remove_scaffolding_builder(self._sv.id)
      end)
   self:_on_active_changed()
end

function ScaffoldingBuilder_OneDim:destroy()
   self:_untrace_blueprint_and_project()
end

function ScaffoldingBuilder_OneDim:get_scaffolding_region()
   return self._sv.scaffolding_rgn
end

function ScaffoldingBuilder_OneDim:get_normal()
   return self._sv.normal
end

function ScaffoldingBuilder_OneDim:get_active()
   return self._sv.active
end

function ScaffoldingBuilder_OneDim:set_active(active)
   checks('self', '?boolean')

   if active ~= self._sv.active then
      self._sv.active = active
      self.__saved_variables:mark_changed()
      self:_on_active_changed()
   end
end

function ScaffoldingBuilder_OneDim:set_mode(mode)
   checks('self', 'string')

   if mode ~= self._sv.mode then
      self._sv.mode = mode
      self.__saved_variables:mark_changed()

      if self._sv.active then
         self:_update_scaffolding_size()
      end
   end
end

function ScaffoldingBuilder_OneDim:_on_active_changed()
   local active = self._sv.active

   if active then
      self:_trace_blueprint_and_project()
      self:_update_scaffolding_size()
   else
      self:_untrace_blueprint_and_project()
   end
   self._sv.manager:_on_active_changed(self._sv.id, active)
end

function ScaffoldingBuilder_OneDim:_untrace_blueprint_and_project()
   if self._project_trace then
      self._project_trace:destroy()
      self._project_trace = nil
   end
   if self._blueprint_trace then
      self._blueprint_trace:destroy()
      self._blueprint_trace = nil
   end
   if self._gameloop_listener then
      self._gameloop_listener:destroy()
      self._gameloop_listener = nil
   end
end

function ScaffoldingBuilder_OneDim:_trace_blueprint_and_project()
   assert(self._sv.active)
   assert(not self._project_trace)
   assert(not self._blueprint_trace)

   local building = build_util.get_building_for(self._sv.entity)

   local function mark_dirty()
      self:_mark_dirty()
   end

   self._project_trace     = self._sv.project_rgn:trace('scaffolding builder'):on_changed(mark_dirty)
   self._blueprint_trace   = self._sv.blueprint_rgn:trace('scaffolding builder'):on_changed(mark_dirty)
   self._building_listener = radiant.events.listen(building, 'stonehearth:construction:structure_finished_changed', mark_dirty)
end

function ScaffoldingBuilder_OneDim:_mark_dirty()
   if not self._gameloop_listener then
      self._gameloop_listener = radiant.events.listen_once(radiant, 'stonehearth:gameloop', function()
            self._gameloop_listener = nil
            self:_update_scaffolding_size()
         end )
   end
end

function ScaffoldingBuilder_OneDim:_update_scaffolding_size()
   local teardown = self._sv.mode == ScaffoldingBuilder_OneDim.TEAR_DOWN
   
   if not teardown then
      if not self:_get_blueprint_finished() then
         -- still not done with the blueprint.  cover the whole thing
         self:_cover_project_region(teardown)
      else
         -- wipe it out
         self._sv.scaffolding_rgn:modify(function(cursor)
               cursor:clear()
            end)
      end
   else
      self:_cover_project_region(teardown)
   end
   self:_update_ladder_region()
end

function ScaffoldingBuilder_OneDim:_get_blueprint_finished()
   if not self._sv.entity:is_valid() then
      return
   end

   local building = build_util.get_building_for(self._sv.entity)
   local all_structures = building:get_component('stonehearth:building')
                                       :get_all_structures()

   for i, name in pairs(ScaffoldingBuilder_OneDim.SOLID_STRUCTURE_TYPES) do
      for _, entry in pairs(all_structures[name]) do
         local finished = entry.entity:get_component('stonehearth:construction_progress')
                                          :get_finished()
         if not finished then
            return false
         end
      end
   end
   return true
end

-- why is this function so big!?
function ScaffoldingBuilder_OneDim:_cover_project_region(teardown)
   local project_rgn = self._sv.project_rgn:get()
   local blueprint_rgn = self._sv.blueprint_rgn:get()
   
   self._sv.scaffolding_rgn:modify(function(cursor)
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
         local blueprint_max_heights = Array2D(size.x, size.z, origin, -INFINITE)
         if not is_grounded then
            blueprint_min_heights = Array2D(size.x, size.z, origin, INFINITE)
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
            local clipper = Cube3(Point3(-INFINITE, project_bounds.min.y, -INFINITE),
                                  Point3( INFINITE, project_bounds.max.y,  INFINITE))

            if not (blueprint_rgn:clipped(clipper) - project_rgn):empty() then
               subtract_top_row = true
            end
         end

         if subtract_top_row then
            project_bounds.max = project_bounds.max - Point3.unit_y
         end

         local max_height = project_bounds.max.y
   
         -- add a piece of scaffolding for every xz column in `bounds`.  the
         -- height of the scaffolding will be the min of the max height and the
         -- previously computed blueprint_max_heights entry for that xz position
         local scaffolding_heights = Array2D(size.x, size.z, origin, -INFINITE)               
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
            local top_row_clipper = Cube3(Point3(-INFINITE, project_bounds.max.y, -INFINITE),
                                          Point3( INFINITE, INFINITE,  INFINITE))
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
               if h ~= -INFINITE then
                  if not is_grounded then
                     -- if the region is floating in the air, we put the base of the
                     -- column at the base of the blueprint
                     assert(blueprint_min_heights)
                     base = blueprint_min_heights:get(x, z)
                     assert(base ~= INFINITE)
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

function ScaffoldingBuilder_OneDim:_update_ladder_region()
   self._sv.manager:_on_scaffolding_region_changed(self._sv.id)
end

return ScaffoldingBuilder_OneDim
