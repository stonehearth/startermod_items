local build_util = require 'lib.build_util'
local constants = require('constants').construction

local Building = class()
local Rect2 = _radiant.csg.Rect2
local Cube3 = _radiant.csg.Cube3
local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3
local Region2 = _radiant.csg.Region2
local Region3 = _radiant.csg.Region3
local Array2D = _radiant.csg.Array2D
local TraceCategories = _radiant.dm.TraceCategories

local INFINITE = 10000000

-- all structure types, and a table containing them to make iteration handy.
local WALL = 'stonehearth:wall'
local COLUMN = 'stonehearth:column'
local ROOF = 'stonehearth:roof'
local FLOOR = 'stonehearth:floor'
local FIXTURE_FABRICATOR = 'stonehearth:fixture_fabricator' -- xx: rename this fixture...

local STRUCTURE_TYPES = {
   WALL,
   COLUMN,
   ROOF,
   FLOOR,
   FIXTURE_FABRICATOR,
}

-- normals in the x and z directions.  used for creating patch walls.
local Z_NORMALS = {
   Point3( 0,  0,  1),
   Point3( 0,  0, -1),
}
local X_NORMALS = {
   Point3( 1,  0,  0),
   Point3(-1,  0,  0),
}

function Building:initialize(entity, json)
   self._entity = entity
   self._log = radiant.log.create_logger('build')
                              :set_prefix('building')
                              :set_entity(entity)

  if not self._sv.initialized then
      self._sv.initialized = true
      self._sv.structures = {}
      for _, structure_type in pairs(STRUCTURE_TYPES) do
         self._sv.structures[structure_type] = {}
      end
   else
      radiant.events.listen_once(radiant, 'radiant:game_loaded', function()
            self:_restore_structure_traces()
         end)      
   end
   self:_trace_entity_container()
   self._traces = {}
   self._cp_listeners = {}
end

function Building:destroy()
   if self._ec_trace then
      self._ec_trace:destroy()
      self._ec_trace = nil
   end
   radiant.entities.destroy_entity(self._sv.envelope_entity)
end

function Building:_restore_structure_traces()
   for _, structures in pairs(self._sv.structures) do
      for _, entry in pairs(structures) do
         self:_trace_entity(entry.entity, true)
      end
   end
end

function Building:_get_structures(type)
   return self._sv.structures[type]
end

function Building:get_all_structures()
   return self._sv.structures
end

function Building:get_floors(floor_type)
   local result = {}
   for _, entry in pairs(self._sv.structures[FLOOR]) do
      if entry.structure:get_category() == floor_type or floor_type == nil then
         table.insert(result, entry.entity)
      end
   end
   return result
end

function Building:calculate_floor_region()
   local floor_region = Region2()
   for _, entry in pairs(self._sv.structures[FLOOR]) do
      local floor_entity = entry.entity
      local rgn = floor_entity:get_component('destination'):get_region():get()
      for cube in rgn:each_cube() do
         local rect = Rect2(Point2(cube.min.x, cube.min.z),
                            Point2(cube.max.x, cube.max.z))
         floor_region:add_unique_cube(rect)
      end
   end

   -- we need to construct the most optimal region possible.  since floors are colored,
   -- the floor_region may be extremely complex (at worst, 1 cube per point!).  to get
   -- the best result, iterate through all the points in the region in an order in which
   -- the merge-on-add code in the region backend will build an extremly optimal region.
   local optimized_region = Region2()
   local bounds = floor_region:get_bounds()
   local pt = Point2()
   local try_merge = false
   for x=bounds.min.x,bounds.max.x do
      pt.x = x
      for y=bounds.min.y,bounds.max.y do
         pt.y = y
         if floor_region:contains(pt) then
            optimized_region:add_point(pt)
            if try_merge then
               optimized_region:optimize_by_merge()
            end
         else
            try_merge = true
         end
      end
   end

   return optimized_region
end

function Building:add_structure(entity)
   -- Lazily build envelope entity, because it might actually have been created when we loaded
   -- this building from a template.
   if not self._sv.envelope_entity then
      self._sv.envelope_entity = radiant.entities.create_entity('', { owner = self._entity })
      self._sv.envelope_entity:set_debug_text(string.format('envelope for %s', tostring(self._entity)))
      self._sv.envelope_entity:add_component('stonehearth:no_construction_zone')
                                 :set_building_entity(self._entity)
      radiant.entities.add_child(self._entity, self._sv.envelope_entity)
   end

   local id = entity:get_id()

   self._log:detail('adding structure %s to building', entity)

   for _, structure_type in pairs(STRUCTURE_TYPES) do
      local trace
      local structure = entity:get_component(structure_type)

      if structure then
         self._sv.structures[structure_type][id] = {
            entity = entity,
            structure = structure,
         }

         if structure_type == ROOF then
            self:_create_roof_dependencies(entity)
         elseif structure_type == FLOOR then
            self:_create_floor_dependencies(entity)
         elseif structure_type == WALL then
            self:_create_wall_dependencies(entity)
         end
         self:_trace_entity(entity)
         self._sv.envelope_entity:get_component('stonehearth:no_construction_zone')
                                       :add_structure(entity)         
         structure:layout()
         break
      end
   end
   self.__saved_variables:mark_changed()
end

function Building:_remove_structure(entity)
   local id = entity:get_id()
   local layout_roof = false

   -- remove the entity from our structure map.  it appears only once, we just have
   -- to find the right table in our list...
   for _, structure_type in pairs(STRUCTURE_TYPES) do
      local trace
      local structure = entity:get_component(structure_type)
      if structure then
         self._sv.structures[structure_type][id] = nil
         self.__saved_variables:mark_changed()
         if structure_type == ROOF then
            layout_roof = true
            self:_remove_roof_dependencies(entity)
         elseif structure_type == FLOOR then
            self:_remove_floor_dependencies(entity)
         elseif structure_type == WALL then
            self:_remove_wall_dependencies(entity)
         end
         break
      end      
   end

   -- if there are any traces for this entity, nuke them now, too.
   self:_untrace_entity(id)

   if layout_roof then
      self:layout_roof()
   end
end

function Building:_create_wall_dependencies(wall)
   local wall_cp = wall:get_component('stonehearth:construction_progress')
   for _, entry in pairs(self._sv.structures[FLOOR]) do
      local floor = entry.entity
      wall_cp:add_dependency(floor)
   end
   for _, entry in pairs(self._sv.structures[ROOF]) do
      local roof = entry.entity
      wall_cp:loan_scaffolding_to(roof)      
   end   
end

function Building:_remove_wall_dependencies(wall)
end

function Building:_create_floor_dependencies(floor)
   for _, entry in pairs(self._sv.structures[WALL]) do
      local wall = entry.entity
      wall:get_component('stonehearth:construction_progress')
               :add_dependency(floor)
   end
end

function Building:_remove_floor_dependencies(floor)
end

function Building:_create_roof_dependencies(roof)
   for _, entry in pairs(self._sv.structures[WALL]) do
      local structure = entry.entity

      -- don't build the roof until we've built all the supporting structures
      roof:add_component('stonehearth:construction_progress')
                  :add_dependency(structure)

      -- loan out some scaffolding
      structure:get_component('stonehearth:construction_progress')
                  :loan_scaffolding_to(roof)
   end
end

function Building:_remove_roof_dependencies(roof)
   for _, entry in pairs(self._sv.structures[WALL]) do
      local structure = entry.entity
      -- unloan out some scaffolding
      structure:get_component('stonehearth:construction_progress')
                  :unloan_scaffolding_to(roof)
   end
end


function Building:_save_trace(entity, trace)
   local id = entity:get_id()
   if not self._traces[id] then
      self._traces[id] = {}
   end
   table.insert(self._traces[id], trace)
end

function Building:_trace_entity_container()
   local ec = self._entity:add_component('entity_container')
   self._ec_trace = ec:trace_children('auto-destroy building')
                        :on_removed(function()
                              if ec:is_valid() and ec:num_children() == 1 then
                                 local teardown = self._entity:get_component('stonehearth:construction_progress'):get_teardown()
                                 if teardown then
                                    radiant.entities.destroy_entity(self._entity)
                                 end
                              end
                           end)
end

function Building:get_building_footprint()
   return self._sv.envelope_entity:get_component('region_collision_shape')
                                       :get_region()
end

function Building:set_active(enabled)
   if enabled then
      self._sv.envelope_entity:get_component('stonehearth:no_construction_zone')
                                 :clear_traces()
   end
end

function Building:_trace_entity(entity, loading)
   local id = entity:get_id()

   radiant.events.listen_once(entity, 'radiant:entity:pre_destroy', function()
         self:_remove_structure(entity)
      end)

   self._cp_listeners[id] = radiant.events.listen(entity, 'stonehearth:construction:finished_changed', function()
         self:_on_child_finished()
      end)

   if entity:get_component('stonehearth:roof') then
      local trace = entity:get_component('stonehearth:construction_data'):trace_data('layout roof', TraceCategories.SYNC_TRACE)
                              :on_changed(function()
                                    self:layout_roof(entity)
                                 end)
      self:_save_trace(entity, trace)

      if not loading then
         -- if we're loading, our shape is already perfect!  no need to push the state, just
         -- put the trace back
         trace:push_object_state()
      end
   end
end

function Building:_untrace_entity(id)
   local traces = self._traces[id]
   if traces then
      self._traces[id] = nil
      for _, trace in ipairs(traces) do
         trace:destroy()
      end
   end
   local listener = self._cp_listeners[id]
   if listener then
      listener:destroy()
      self._cp_listeners[id] = nil
   end

   if self._sv.envelope_entity:is_valid() then
      self._sv.envelope_entity:get_component('stonehearth:no_construction_zone')
                                 :remove_structure(id)
   end
end

function Building:grow_local_box_to_roof(entity, local_box)
   local p0, p1 = local_box.min, local_box.max

   local shape = Region3(local_box)
   for _, entry in pairs(self._sv.structures[ROOF]) do
      local roof = entry.entity
      local roof_region = roof:get_component('destination'):get_region():get()

      local stencil = Cube3(Point3(p0.x, p1.y, p0.z),
                            Point3(p1.x, INFINITE, p1.z))
      
      -- translate the stencil into the roof's coordinate system, clip it,
      -- then transform the result back to our coordinate system
      local offset = radiant.entities.get_location_aligned(roof) -
                     radiant.entities.get_location_aligned(entity)

      local roof_overhang = roof_region:clipped(stencil:translated(-offset))                                       
                                       :translated(offset)
     
      -- iterate through the overhang and merge shingle that are atop each
      -- other
      local merged_roof_overhang = Region3()
      for shingle in roof_overhang:each_cube() do
         local merged = Cube3(Point3(shingle.min.x, shingle.min.y, shingle.min.z),
                              Point3(shingle.max.x, INFINITE,      shingle.max.z))
         merged_roof_overhang:add_cube(merged)
      end

      -- iterate through each "shingle" in the overhang, growing the shape
      -- upwards toward the base of the shingle.
      for shingle in merged_roof_overhang:each_cube() do
         local col = Cube3(Point3(shingle.min.x, p1.y, shingle.min.z),
                           Point3(shingle.max.x, shingle.min.y, shingle.max.z))
         -- In specific circumstances, (L-shaped room with the vertical part of
         -- the 'L' being only 1-unit 'tall'), and roof configurations (the roof
         -- slopes perpendicular to the vertical part of the 'L'), we get walls
         -- that pass through shingles more than once, and therefore cannot be added
         -- uniquely. This trips an assert/crashes the game, so let's just add
         -- them blindly, for now. -- klochek-rad
         shape:add_cube(col)
      end
   end
   return shape
end

-- links will be put back by the BuildingUndoManager
function Building:unlink_entity(entity)
   self:_remove_structure(entity)
end

function Building:layout_roof(roof)
   -- assert(stonehearth.build:in_transaction())

   -- first, layout the roof
   if roof then
      roof:get_component(ROOF)
                  :layout()
   end

   -- now layout all the walls and columns
   for _, entry in pairs(self._sv.structures[COLUMN]) do
      entry.structure:layout()
   end

   -- layout all the normal walls and create a list of the patch walls.
   local patch_walls = {}
   for _, entry in pairs(self._sv.structures[WALL]) do
      if entry.structure:is_patch_wall() then
         table.insert(patch_walls, entry.entity)
      else         
         entry.structure:layout()
      end
   end

   -- nuke all the existing patch walls and add new ones to cover the gaps
   -- left in the roofing process.
   for _, entity in ipairs(patch_walls) do
      stonehearth.build:unlink_entity(entity)
   end

   if roof then
      self:_add_patch_walls_to_building(roof)
   end
end

-- create 2d arrays for the min and max coordinates of every tile
-- in the roof.
--
function Building:_compute_min_and_max_arrays(region, bounds)
   local origin = Point2(bounds.min.x, bounds.min.z)
   local bounds = region:get_bounds()

   local width  = bounds.max.x - bounds.min.x
   local height = bounds.max.z - bounds.min.z

   local mins = Array2D(width, height, origin,  INFINITE)
   local maxs = Array2D(width, height, origin, -INFINITE)

   for cube in region:each_cube() do
      local y_min = cube.min.y
      local y_max = cube.max.y

      for x = cube.min.x, cube.max.x-1 do
         for z = cube.min.z, cube.max.z-1 do
            mins:set(x, z, math.min(mins:get(x, z), y_min))
            maxs:set(x, z, math.max(maxs:get(x, z), y_max))
         end
      end
   end
   return mins, maxs
end

function Building:_add_patch_walls_to_building(roof)
   local region = roof:get_component('destination'):get_region():get()
   local bounds = region:get_bounds()
   
   -- create mins and maxs heightmaps describing the upper and lower
   -- height bounds for the roof at each x, z location
   local mins, maxs = self:_compute_min_and_max_arrays(region, bounds)

   -- this varaibles are  referenced in local functions by upvalue.  
   -- the local functions help with decomp.
   local current_patch_wall_shape
   local roof_origin = radiant.entities.get_location_aligned(roof)
   
   -- checks the upvalue `current_patch_wall_shape`.  if it's a non-empty region,
   -- add a patch wall with its shape and clear the variable
   local function add_wall_patch(normal)
      if current_patch_wall_shape then
         self:_remove_building_from_region(roof_origin, current_patch_wall_shape)
         if not current_patch_wall_shape:empty() then
            local uri = self:_recommend_patch_wall_material(roof_origin, current_patch_wall_shape)
            stonehearth.build:add_patch_wall_to_building(self._entity, uri, normal, roof_origin, current_patch_wall_shape)
         end
         current_patch_wall_shape = nil
      end
   end
   
   -- the inner loop of the patching process.  we factor it this way so we can share the
   -- logic with row-major and column-major nesting of the outer loops (as determined by
   -- the direction of the normal).  adds all patch walls
   local function inner_loop(x, z, normal)
      local min_height = mins:get(x, z)
      local max_height = maxs:get(x, z)

      -- if the gap is big enough, consider adding a patch wall here
      if min_height < (max_height - 2) then
         -- do we need a wall, though?  look over 1 in the -normal direction and check the
         -- min height against our current min_height.  this is used to distinguish between
         -- the edge just INSIDE the patch wall vs the one just OUTSIDE the wall.
         local sx, sz = x - normal.x, z - normal.z
         local max_height_over_gap = maxs:get(sx, sz, -INFINITE)
         local min_height_over_gap = mins:get(sx, sz,  INFINITE)

         local needs_patch_wall = min_height_over_gap > min_height + 1
         if needs_patch_wall then
            -- craptacular hack!  look at the blocks on either side of the normal.
            -- if they are not both above the min, we're in some freaky edge case where we
            -- do NOT want a wall
            needs_patch_wall = (maxs:get(x + normal.z, z + normal.x, -INFINITE) > -INFINITE) and -- tangent to current
                               (maxs:get(x - normal.z, z - normal.x, -INFINITE) > -INFINITE) and -- other to current
                               (maxs:get(sx + normal.z, sz + normal.x, -INFINITE) > -INFINITE) and -- tangent to gap
                               (maxs:get(sx - normal.z, sz - normal.x, -INFINITE) > -INFINITE) and -- other tangent to gap
                               true
         end
         
         -- either accumulate or finish off the patch wall we've got
         if needs_patch_wall then
            -- no patch wall yet?  create an empty one!
            if not current_patch_wall_shape then
               current_patch_wall_shape = Region3()
            end
            -- add the column for this air-gap to the patch wall
            local column = Cube3(Point3(sx,     min_height + 1, sz),
                                 Point3(sx + 1, max_height - 1, sz + 1))
            current_patch_wall_shape:add_unique_cube(column)
         elseif current_patch_wall_shape then
            -- ran out of wall.  go ahead and add the current one.
            add_wall_patch(normal)
         end
      end
   end
   
   -- loop in row-major order for normals in the x direction, running
   -- the inner_loop function to actually make the patch walls.  failing
   -- to loop in the proper order would break the column merging code in the
   -- inner loop.
   for _, normal in ipairs(X_NORMALS) do
      for x = bounds.min.x, bounds.max.x - 1 do
         for z = bounds.min.z, bounds.max.z - 1 do
            inner_loop(x, z, normal)
         end
      end
      add_wall_patch(normal)
   end
   
   -- now loop in col-major order for normals in the z direction.
   for _, normal in ipairs(Z_NORMALS) do
      for z = bounds.min.z, bounds.max.z - 1 do
         for x = bounds.min.x, bounds.max.x - 1 do
            inner_loop(x, z, normal)
         end
      end
      add_wall_patch(normal)
   end
   
   assert(not current_patch_wall_shape)
end

-- remove the shape of all other parts of the building from the specified region.
-- useful for making sure building parts don't overlap!
--
function Building:_remove_building_from_region(origin, region)
   local bounds = region:get_bounds()

   for _, structures in pairs(self._sv.structures) do
      for _, entry in pairs(structures) do
         local dst = entry.entity:get_component('destination')
         if dst then
            local entity_origin = radiant.entities.get_location_aligned(entry.entity)
            local offset = entity_origin - origin

            local rgn = dst:get_region():get()
            local rgn_bounds = rgn:get_bounds()
            if bounds:intersects(rgn_bounds:translated(offset)) then
               region:subtract_region(rgn:translated(offset))
            end
         end
      end
   end
end

function Building:_recommend_patch_wall_material(origin, shape)
   local closest_d
   local recommended = 'stonehearth:wooden_wall'
   local bounds = shape:get_bounds()

   for _, entry in pairs(self._sv.structures[WALL]) do
      local dst = entry.entity:get_component('destination')
      if dst then
         local entity_origin = radiant.entities.get_location_aligned(entry.entity)
         local offset = entity_origin - origin

         local rgn = dst:get_region():get()
         local rgn_bounds = rgn:get_bounds()
                                    :translated(offset)
         local d = bounds:distance_to(rgn_bounds:translated(offset))
         if not closest_d or d < closest_d then
            recommended = entry.entity:get_uri()
            closest_d = d
            if closest_d == 0 then
               break
            end
         end
      end
   end
   return recommended
end


-- fires when a child finishes construction.  building's cannot use the dependenc/inverse_dependncy
-- system to figure out when they're finished.  regardless of whether we're building up or tearing
-- down, the entire building isn't finished until all of the children inside the building are
-- finished.  manually crawl our entire entity tree to compute that, then update our contruction
-- progress component
--
function Building:_on_child_finished(changed)
   local function children_finished(entity)
      if entity and entity:is_valid() then
         if entity ~= self._entity then
            local cp = entity:get_component('stonehearth:construction_progress')
            if cp and not cp:get_finished() then
               self._log:spam('%s is not finished!  stopping search.', entity)
               return false
            end
         end
         
         local ec = entity:get_component('entity_container')  
         if ec then
            for id, child in ec:each_child() do
               if not children_finished(child) then
                  return false
               end
            end
         end
      end
      return true
   end
   
   if not self._entity:is_valid() then
      return
   end
   self._log:spam('checking to see if all children are finished...')

   local finished = children_finished(self._entity)
   self._entity:get_component('stonehearth:construction_progress')
                     :set_finished(finished)
end

function Building:save_to_template()
   local structures = {}
   for structure_type, entries in pairs(self._sv.structures) do
      structures[structure_type] = radiant.keys(entries)
   end
   
   return {
      structures = structures,
      envelope_entity = build_util.pack_entity(self._sv.envelope_entity),
   }
end

function Building:load_from_template(template, options, entity_map)
   self._sv.structures = {}
   for structure_type, ids in pairs(template.structures) do
      local structures = {}
      self._sv.structures[structure_type] = structures
      for _, id in ipairs(ids) do
         local entity = entity_map[id]
         assert(entity)

         structures[entity:get_id()] = {
            entity = entity,
            structure = entity:add_component(structure_type),
         }
      end
   end
   self._sv.envelope_entity = build_util.unpack_entity(template.envelope_entity, entity_map)
   self._sv.envelope_entity:set_debug_text(string.format('envelope for %s', tostring(self._entity)))

   self.__saved_variables:mark_changed()
end

function Building:finish_restoring_template()
   self:_restore_structure_traces()

   self._sv.envelope_entity:add_component('stonehearth:no_construction_zone')
                              :finish_restoring_template()

   for _, structures in pairs(self._sv.structures) do
      for _, entry in pairs(structures) do
         local cp = entry.entity:get_component('stonehearth:construction_progress')
         if cp then
            cp:finish_restoring_template()
         end
      end
   end
end

return Building
