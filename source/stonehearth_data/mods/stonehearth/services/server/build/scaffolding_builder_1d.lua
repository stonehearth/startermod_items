local priorities = require('constants').priorities.simple_labor
local build_util = require 'stonehearth.lib.build_util'

local Cube3   = _radiant.csg.Cube3
local Point2  = _radiant.csg.Point2
local Point3  = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local Array2D = _radiant.csg.Array2D
local TraceCategories = _radiant.dm.TraceCategories

local INFINITE = 1000000
local CLIP_SOLID = _radiant.physics.Physics.CLIP_SOLID

local ScaffoldingBuilder_OneDim = class()

ScaffoldingBuilder_OneDim.SOLID_STRUCTURE_TYPES = {
   'stonehearth:roof',
   'stonehearth:wall',
   'stonehearth:column',
   'stonehearth:floor',
}

ScaffoldingBuilder_OneDim.BUILD = 'build'
ScaffoldingBuilder_OneDim.TEAR_DOWN = 'teardown'

function ScaffoldingBuilder_OneDim:initialize(manager, id, entity, blueprint_rgn, project_rgn, normal, stand_at_base)
   checks('self', 'controller', 'number', 'Entity', 'Region3Boxed', 'Region3Boxed', '?Point3', 'boolean')

   self._sv.id = id
   self._sv.active = false
   self._sv.entity = entity
   self._sv.preferred_normal = normal
   self._sv.manager = manager
   self._sv.stand_at_base = stand_at_base
   self._sv.project_rgn = project_rgn
   self._sv.blueprint_rgn = blueprint_rgn
   self._sv.origin = radiant.entities.get_world_grid_location(entity)
   self._sv.mode = ScaffoldingBuilder_OneDim.BUILD

   self._sv.scaffolding_rgn = _radiant.sim.alloc_region3()   
end

function ScaffoldingBuilder_OneDim:activate()
   local entity = self._sv.entity
   local name = radiant.entities.get_display_name(entity)

   self._debug_text = ''
   if name then
      self._debug_text = self._debug_text .. ' ' .. name
   else
      self._debug_text = self._debug_text .. ' ' .. entity:get_uri()
   end
   local debug_text = entity:get_debug_text()
   if debug_text then
      self._debug_text = self._debug_text .. ' ' .. debug_text
   end

   self._log = radiant.log.create_logger('build.scaffolding.1d')
                              :set_entity(entity)
                              :set_prefix('s1d:' .. tostring(self._sv.id) .. ' ' .. self._debug_text)

   radiant.events.listen(self._sv.entity, 'radiant:entity:pre_destroy', function()
         self:_remove_scaffolding_region()
      end)

   if self._sv.active then
      self:_trace_blueprint_and_project()

      -- for performance reasons, we don't update our regions synchronously.  this creates a race
      -- where our region may be out of date when the application is closed by the user (or crashes!).
      -- so if we're restoring while active, unconditionally mark us as dirty.
      self:_mark_dirty()
   end
end

function ScaffoldingBuilder_OneDim:destroy()
   self._log:info('destroying scaffolding builder')
   self:_untrace_blueprint_and_project()
   self._sv.scaffolding_rgn = nil
end

function ScaffoldingBuilder_OneDim:get_id()
   return self._sv.id
end

-- interfaces for the owner of the builder
function ScaffoldingBuilder_OneDim:set_clipper(blueprint_clipbox)
   self._sv.blueprint_clipbox = blueprint_clipbox
end

function ScaffoldingBuilder_OneDim:set_active(active)
   checks('self', '?boolean')

   if active ~= self._sv.active then
      self._log:detail('active changed to %s', active)
      self._sv.active = active
      self.__saved_variables:mark_changed()
      self:_update_status()
   end
end

function ScaffoldingBuilder_OneDim:set_teardown(teardown)
   checks('self', 'boolean')
   local mode = teardown and 'teardown' or 'build'

   if mode ~= self._sv.mode then
      self._log:detail('teardown changed to %s', teardown)
      self._sv.mode = mode
      self.__saved_variables:mark_changed()

      if self._sv.active then
         self:_update_scaffolding_size()
      end
   end
end

function ScaffoldingBuilder_OneDim:_add_scaffolding_region()
   self._sv.manager:_add_region(self._sv.id,
                                self._debug_text,
                                self._sv.entity,
                                self._sv.origin,
                                self._sv.blueprint_rgn,
                                self._sv.blueprint_clipbox,
                                self._sv.scaffolding_rgn,
                                self._sv.normal)
end

function ScaffoldingBuilder_OneDim:_remove_scaffolding_region()
   self._sv.manager:_remove_region(self._sv.id)
   self._sv.manager:_remove_builder(self._sv.id)
end

function ScaffoldingBuilder_OneDim:_update_status()
   local active = self._sv.active

   if active then
      self:_choose_normal()
      self:_trace_blueprint_and_project()
      self:_update_scaffolding_size()
      self:_add_scaffolding_region()
   else
      self:_untrace_blueprint_and_project()
      self:_remove_scaffolding_region()
   end
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
   if self._building_listener then
      self._building_listener:destroy()
      self._building_listener = nil
   end
end

function ScaffoldingBuilder_OneDim:_trace_blueprint_and_project()
   assert(self._sv.entity)
   assert(self._sv.active)
   assert(not self._project_trace)
   assert(not self._blueprint_trace)

   local building = build_util.get_building_for(self._sv.entity)
   assert(building)

   local function mark_dirty()
      self:_mark_dirty()
   end

   self._project_trace     = self._sv.project_rgn:trace('scaffolding builder'):on_changed(mark_dirty)
   self._blueprint_trace   = self._sv.blueprint_rgn:trace('scaffolding builder'):on_changed(mark_dirty)
   self._building_listener = radiant.events.listen(building, 'stonehearth:construction:structure_finished_changed', mark_dirty)
end

function ScaffoldingBuilder_OneDim:_mark_dirty()
   assert(self._sv.scaffolding_rgn)
   if not self._gameloop_listener then
      self._gameloop_listener = radiant.events.listen_once(radiant, 'stonehearth:gameloop', function()
            self._gameloop_listener = nil
            if self._sv.active then
               self:_update_scaffolding_size()
            end
         end)
   end
end

function ScaffoldingBuilder_OneDim:_update_scaffolding_size()
   local teardown = self._sv.mode == ScaffoldingBuilder_OneDim.TEAR_DOWN

   self._log:spam('updating scaffolding size')
   
   if self:_building_is_finished() then
      self._log:spam('building is finished.  erasing scaffolding region')
      self._sv.scaffolding_rgn:modify(function(cursor)
            cursor:clear()
         end)
      return
   end

   self:_cover_project_region(teardown)
end

function ScaffoldingBuilder_OneDim:_building_is_finished()
   if not self._sv.entity:is_valid() then
      return
   end

   local building = build_util.get_building_for(self._sv.entity)

   local all_structures = building:get_component('stonehearth:building')
                                       :get_all_structures()

   -- only check the things that need scaffolding.
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

function ScaffoldingBuilder_OneDim:_cover_project_region(teardown)
   local normal = self._sv.normal
   local blueprint_clipbox = self._sv.blueprint_clipbox
   local stand_at_base = self._sv.stand_at_base

   local project_rgn = self._sv.project_rgn:get()
   local blueprint_rgn = self._sv.blueprint_rgn:get()

   -- the clip box is used to convert a 3d region to a 2d one which
   -- is flat and normal to the normal.
   self._log:spam('blueprint:%s  project:%s', blueprint_rgn:get_bounds(), project_rgn:get_bounds())
   if blueprint_clipbox then
      project_rgn   = project_rgn:intersect_cube(blueprint_clipbox)
      blueprint_rgn = blueprint_rgn:intersect_cube(blueprint_clipbox)
      self._log:spam('(clipped) blueprint:%s  project:%s', blueprint_rgn:get_bounds(), project_rgn:get_bounds())
   end

   -- compute the top of the project.  the `top` is the height
   -- of the completely finished part of the project
   local blueprint_bounds = blueprint_rgn:get_bounds()
   local blueprint_top = blueprint_bounds.max.y

   local project_top
   if stand_at_base then
      project_top = blueprint_bounds.min.y
      self._log:spam('standing at base.  using %d for project_top', project_top)
   elseif not project_rgn:empty() then
      local project_bounds = project_rgn:get_bounds()
      project_top = project_bounds.max.y
      local toprow = Cube3(Point3(-INFINITE, project_top - 1, -INFINITE),
                           Point3( INFINITE, project_top,      INFINITE))
      local remaining_blocks = blueprint_rgn:clipped(toprow) - project_rgn:clipped(toprow)
      if not remaining_blocks:empty() then
         project_top = project_top - 1
      end
      project_top = math.min(project_top, blueprint_top - 1)
      self._log:spam('project is not empty.  using %d for project_top', project_top)
   else
      project_top = blueprint_bounds.min.y
      self._log:spam('project is empty.  using %d for project_top', project_top)
   end
   assert(project_top < blueprint_top)
   
   -- if we're tearing down, we may need to drop the top by 1.
   if teardown and project_top > 0 then
      local top_row_clipper = Cube3(Point3(-INFINITE, project_top,     -INFINITE),
                                    Point3( INFINITE, project_top + 1,  INFINITE))
      if project_rgn:clipped(top_row_clipper):empty() then
         project_top = project_top - 1
      end
   end

   -- snip off the unreachable upper part of the blueprint.
   local clipper = Cube3(Point3(-INFINITE, -INFINITE,       -INFINITE),
                         Point3( INFINITE, project_top + 1,  INFINITE))
   local reachable_blueprint = blueprint_rgn:clipped(clipper)


   -- if there are holes in the blueprint (e.g. for doors or windows), we need
   -- to make sure we plug them up.  the projection to CLIP_SOLID (below) will
   -- take care of most of them, but we do have to make sure the very top row
   -- is taken care of.  Consider what happens when trying to build scaffolding
   -- 1/2 way up a peaked roof.  We'll end up with two towers of scaffolding
   -- growing toward one another unless we close that gap.
   local bounds = reachable_blueprint:get_bounds()
   local start = Point3(bounds.min.x, bounds.max.y - 1, bounds.min.z)
   local finish = bounds.max
   local top_row = reachable_blueprint:clipped(Cube3(start, finish))

   reachable_blueprint:add_cube(top_row:get_bounds())


   -- starting 1 row down and 1 row out, all the way till we find terrain
   reachable_blueprint:translate(-Point3.unit_y + normal)

   -- and clip out the terrain
   local origin = self._sv.origin
   reachable_blueprint:translate(origin)
   local region = _physics:project_region(reachable_blueprint, CLIP_SOLID)
   region = _physics:clip_region(region, CLIP_SOLID)
   region:translate(-origin)

   -- finally, make sure we don't build scaffolding on top of things which
   -- have been built in the past.  for example, if we're a slab connected
   -- to a wall to form a balcony, don't build scaffolding for the edge
   -- which is connected to the wall.  building scaffolding for things which
   -- must be built in the future is ok.  otherwise we wouldn't be able to
   -- complete the wall that the slab is attached to!  this makes scaffolding
   -- somewhat magical from a collision perspective, but oh well!  people
   -- can already walk right through it, so why not other building parts, too?
   build_util.clip_dependant_regions_from(self._sv.entity, origin, region)

   -- copy into the cursor in one big :modify() at the end
   self._sv.scaffolding_rgn:modify(function(cursor)
         cursor:copy_region(region)
      end)
end

function ScaffoldingBuilder_OneDim:_choose_normal()
   -- if we have a preferred normal, use that and its opposite.
   -- otherwise, consider all 4 normals
   self._log:detail('choosing normal')

   local normals
   local preferred_normal = self._sv.preferred_normal
   if preferred_normal then
      normals = { preferred_normal, preferred_normal:scaled(-1) }
      self._sv.normal = preferred_normal
      return
   else
      normals = { Point3(0, 0, 1), Point3(0, 0, -1), Point3(1, 0, 0), Point3(-1, 0, 0)}
   end

   -- run through the list, looking for the perfect normal
   local origin = self._sv.origin
   local blueprint_clipbox = self._sv.blueprint_clipbox

   local blueprint_rgn = self._sv.blueprint_rgn:get()
   if blueprint_clipbox then
      blueprint_rgn = blueprint_rgn:intersect_cube(blueprint_clipbox)
   end

   for _, normal in pairs(normals) do
      local good = true
      local world_region = blueprint_rgn:translated(self._sv.origin + normal)
      local unblocked_region = _physics:clip_region(world_region, CLIP_SOLID)

      -- if anything blocks the proposed box, don't bother.
      if unblocked_region:get_bounds().min.y ~= world_region:get_bounds().min.y then
         self._log:detail('normal %s is blocked by world.  rejecting.', normal)
         good = false
      end
      if good then
         -- if there are any blueprint fabricators in the way, there *will* be something
         -- blocking this direction in the future, so stay away from it
         local obstructing = radiant.terrain.get_entities_in_region(unblocked_region);
         for id, entity in pairs(obstructing) do
            if build_util.is_fabricator(entity) then
               self._log:detail('normal %s intersects fabricator %s.  rejecting', normal, entity)
               good = false
               break
            end
         end
      end
      if good then
         self._log:detail('normal %s is all good!  using it.', normal)
         self._sv.normal = normal
         return
      end
   end
   -- gotta pick one!
   self._log:detail('choosing normal %s of last resort', normals[1])
   self._sv.normal = normals[1]
end

return ScaffoldingBuilder_OneDim
