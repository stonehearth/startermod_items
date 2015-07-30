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
   self._log = radiant.log.create_logger('build.building')
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

   -- Just remove the traces. The child entities will be destroyed as part of the object hierarchy
   if self._cp_listeners then
      for id in pairs(self._cp_listeners) do
         self:_untrace_entity(id)
      end
   end

   if self._traces then
      assert(not next(self._traces))
   end

   if self._cp_listeners then
      assert(not next(self._cp_listeners))
   end
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


function Building:each_dependency(blueprint)
   checks('self', 'Entity')
   assert(build_util.is_blueprint(blueprint), string.format('%s is not a blueprint', tostring(blueprint)))

   local entry = self:_get_entry_for_structure(blueprint)
   assert(entry, string.format('blueprint %s is not in building', tostring(blueprint)))

   local id, entity
   return function()
      id, entity = next(entry.dependencies, id)
      return id, entity
   end
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
               optimized_region:optimize_by_merge('building:calculate_floor_region')
            end
         else
            try_merge = true
         end
      end
   end

   return optimized_region
end

function Building:add_structure(entity)
   local id = entity:get_id()
   self._log:detail('adding structure %s to building', entity)

   for _, structure_type in pairs(STRUCTURE_TYPES) do
      local trace
      local structure = entity:get_component(structure_type)

      if structure then
         self._sv.structures[structure_type][id] = {
            entity = entity,
            structure = structure,
            dependencies = {},
            inverse_dependencies = {},
         }
         self:_trace_entity(entity)
         structure:layout()
         break
      end
   end
   self.__saved_variables:mark_changed()
end

function Building:_remove_structure(entity)
   local id = entity:get_id()
   local layout_roof = false
   local entry

   self._log:detail('removing structure %s', entity)

   -- remove the entity from our structure map.  it appears only once, we just have
   -- to find the right table in our list...
   for structure_type, structures in pairs(self._sv.structures) do
      entry = structures[id]
      if entry then
         structures[id] = nil
         if structure_type == ROOF then
            layout_roof = true
         end

         -- fake a finish since the entity is about to go away
         if self._sv.active then
            self:_on_child_finished(entry)
         end
         break
      end
   end

   -- if there are any traces for this entity, nuke them now, too.
   self:_untrace_entity(id)

   if not self._sv.active and layout_roof then
      self:layout_roof()
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
                              if ec:is_valid() and ec:num_children() == 0 then
                                 local teardown = self._entity:get_component('stonehearth:construction_progress'):get_teardown()
                                 if teardown then
                                    radiant.entities.destroy_entity(self._entity)
                                 end
                              end
                           end)
end

-- the building envelope is a Region3 that's the sum of the Region3's of all
-- the building blueprints.
function Building:get_building_envelope(kind, ignoring_blueprint)
   checks('self', 'string', '?Entity')

   local envelope = Region3()
   local origin = radiant.entities.get_world_grid_location(self._entity)
   for _, str_kind in ipairs(STRUCTURE_TYPES) do
      for id, entry in pairs(self._sv.structures[str_kind]) do
         local entity = entry.entity
         if entity ~= ignoring_blueprint then
            local component
            if kind == 'project' then
               component = entity:get_component('region_collision_shape')
            else
               component = entity:get_component('destination')
            end
            if component then
               local rgn = component:get_region()
               if rgn then
                  local location = radiant.entities.get_world_grid_location(entity)
                  local offset = location - origin
                  envelope:add_region(rgn:get()
                                           :translated(offset))
               end
            end
         end
      end
   end
   return envelope
end

function Building:set_active(active)
   self._sv.active = active
   if active then
      self:_compute_dependencies()
      self:_compute_inverse_dependencies()
   end
   self.__saved_variables:mark_changed()
end

function Building:_trace_entity(entity, loading)
   local id = entity:get_id()

   local destroy_trace = radiant.events.listen_once(entity, 'radiant:entity:pre_destroy', function()
         self:_remove_structure(entity)
      end)
   self:_save_trace(entity, destroy_trace)

   self._cp_listeners[id] = radiant.events.listen(entity, 'stonehearth:construction:finished_changed', function(e)
         local entry = self:_get_entry_for_structure(e.entity)
         self:_on_child_finished(entry)
      end)

   --[[
   if entity:get_component('stonehearth:roof') then
      local trace = entity:get_component('stonehearth:roof'):trace_data('layout roof', TraceCategories.SYNC_TRACE)
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
   ]]
end

function Building:_untrace_entity(id)
   local traces = self._traces[id]
   if traces then
      for _, trace in ipairs(traces) do
         trace:destroy()
      end
      self._traces[id] = nil
   end
   local listener = self._cp_listeners[id]
   if listener then
      listener:destroy()
      self._cp_listeners[id] = nil
   end
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

      -- finally once every structure has adjusted its size to cope
      -- with the new configuration of the roof, remove parts of the
      -- roof which still overlap structures
      roof:get_component(ROOF)
            :clip_overlapping_structures()
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
            local brush = self:_recommend_patch_wall_material(roof_origin, current_patch_wall_shape)
            stonehearth.build:add_patch_wall_to_building(self._entity, brush, normal, roof_origin, current_patch_wall_shape)
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
   local recommended = constants.DEFAULT_WOOD_WALL_BRUSH
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
            recommended = entry.structure:get_brush()
            closest_d = d
            if closest_d == 0 then
               break
            end
         end
      end
   end
   return recommended
end


-- fires when a child finishes construction.  building's cannot use the dependency/inverse_dependency
-- system to figure out when they're finished.  regardless of whether we're building up or tearing
-- down, the entire building isn't finished until all of the children inside the building are
-- finished.  manually crawl our entire entity tree to compute that, then update our contruction
-- progress component
--
function Building:_on_child_finished(entry)
   assert(entry)

   local teardown = self._entity:get_component('stonehearth:construction_progress')
                                    :get_teardown()

   -- if we're building, we need to notify everyone who depends on us that it might be
   -- ok for them to start.  if we're tearing down, notify everyone we depend on.
   -- use sync triggers, since the entity might be going away soon!
   self._log:detail('structure %s is finished.  sending notifications', entry.entity)
   local to_notify = teardown and entry.dependencies or entry.inverse_dependencies

   for id, entity in pairs(to_notify) do
      self._log:detail(' -- notifying %s', entity)
      radiant.events.trigger(entity, 'stonehearth:construction:dependencies_finished_changed')
   end

   radiant.events.trigger(self._entity, 'stonehearth:construction:structure_finished_changed')
   self:_update_building_finished()
end

function Building:_update_building_finished(changed)
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
      structures = structures
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
   self.__saved_variables:mark_changed()
end

function Building:finish_restoring_template()
   if not radiant.is_server then
      return
   end
   self:_restore_structure_traces()

   for _, structures in pairs(self._sv.structures) do
      for _, entry in pairs(structures) do
         local cp = entry.entity:get_component('stonehearth:construction_progress')
         if cp then
            cp:finish_restoring_template()
         end
      end
   end
end

function Building:can_start_blueprint(entity, teardown)
   checks('self', 'Entity', 'boolean')

   local entry = self:_get_entry_for_structure(entity)
   if not entry then
      return false
   end
   assert(entry.dependencies)
   assert(entry.inverse_dependencies)

   local dependencies = teardown and entry.inverse_dependencies or entry.dependencies

   self._log:detail('checking to see if %s @ %s can start (teardown:%s)', entity, radiant.entities.get_location_aligned(entity), teardown)
   for id, dependency in pairs(dependencies) do
      if not dependency:is_valid() then
         self._log:detail('   - dependency has been destroyed.  must be finished!');
      elseif not build_util.blueprint_is_finished(dependency) then
         self._log:detail('   - %s @ %s not finished!  no', dependency, radiant.entities.get_location_aligned(dependency))
         return false
      else
         self._log:detail('   - %s @ %s finished!', dependency, radiant.entities.get_location_aligned(dependency))
      end
   end
   self._log:detail('yay!!')
   return true
end


function Building:_add_support_dependencies(entry)
   -- all blueprints must have everything supporting it finished before
   -- starting.
   local blueprint = entry.entity
   local footprint = build_util.get_footprint_region3(blueprint)
   footprint = radiant.entities.local_to_world(footprint, blueprint)

   -- if there's a normal, also depend on the structures in that direction.
   -- for example, this ensure that patch walls on top of roofs depend
   -- on the little piece of roof that the hearthling needs to stand on to
   -- build it.
   local cd = entry.entity:get_component('stonehearth:construction_data')
   if cd then
      local normal = cd:get_normal()
      if normal then
         footprint:add_region(footprint:translated(normal))
      end
   end

   radiant.terrain.get_entities_in_region(footprint, function(e)
         if build_util.is_blueprint(e) then
            local building = build_util.get_building_for(e)
            if building == self._entity then
               self:_add_dependency(entry, e)
            end
         end
      end)
end

function Building:_add_dependency(entry, dependency)
   -- wire both the dependency and the inverse dependency
   local id = dependency:get_id()
   local inverse_dependency = entry.entity
   entry.dependencies[id] = dependency

   for _, structures in pairs(self._sv.structures) do
      local ientry = structures[id]
      if ientry then
         ientry.inverse_dependencies[inverse_dependency:get_id()] = inverse_dependency
         break
      end
   end
end

function Building:_compute_dependencies()
   if self._sv.compute_dependencies_finished then
      return
   end
   self:_compute_common_dependencies()
   self:_compute_wall_dependencies()
   self:_compute_floor_dependencies()
   self:_compute_fixture_dependencies()
end

function Building:_compute_common_dependencies()
   -- these guys are easy.  note that FIXTURE_FABRICATORS have
   -- no region, so they are omitted.
   for _, kind in pairs({ FLOOR, ROOF, COLUMN, WALL }) do
      for _, entry in pairs(self._sv.structures[kind]) do
         self:_add_support_dependencies(entry)
      end
   end
end

function Building:_compute_floor_dependencies()
   for _, entry in pairs(self._sv.structures[FLOOR]) do
      local floor = entry.entity
      -- if there's nothing already supporting the floor, add a dependency
      -- on the walls all around it.  this happens when people use the
      -- slab tool to build balconies hanging off walls.    
      if radiant.empty(entry.dependencies) then
         local floor_origin = radiant.entities.get_world_grid_location(floor)
         local query_region = floor:get_component('destination')
                                       :get_region()
                                          :get()
                                             :inflated(Point3(1, 0, 1))
                                                :translated(Point3(0, -1, 0))
         for _, wall_entry in pairs(self._sv.structures[WALL]) do
            local wall = wall_entry.entity
            local wall_origin = radiant.entities.get_world_grid_location(wall)
            local offset = wall_origin - floor_origin
            local wall_region = wall:get_component('destination')
                                       :get_region()
                                          :get()
                                             :translated(offset)
            if query_region:intersects_region(wall_region) then
               self._log:info('adding connected wall %s dependency to floating floor %s', wall, floor)
               self:_add_dependency(entry, wall)
            end
         end
      end

      -- everything connected to the floor depends on the floor.  for example,
      -- when we use the "grow walls" tool to add walls to some floor, those
      -- walls end up being connected to the floor.  if that was on the ground
      -- level, they might not be directly on top of it!
      local connected = floor:get_component('stonehearth:floor')
                                 :get_connected()

      for _, entity in pairs(connected) do
         local centry = self:_get_entry_for_structure(entity)
         assert(centry)
         self:_add_dependency(centry, floor)
      end

      -- if we depend on any columns, also depend on the walls connected
      -- to them.  this handles the case where a floating floor is supported
      -- only by columns
      local columns = {}
      for id, dependency in pairs(entry.dependencies) do
         local column = dependency:get_component('stonehearth:column')
         if column then
            table.insert(columns, column)
         end
      end
      for _, column in ipairs(columns) do
         for _, wall in pairs(column:get_connected_walls()) do
            self:_add_dependency(entry, wall)
         end
      end
   end
end

function Building:_compute_wall_dependencies()
   for _, entry in pairs(self._sv.structures[WALL]) do
      -- walls depend on their connected columns
      local wall = entry.entity
      local wc = wall:get_component(WALL)
      local col_a, col_b = wc:get_columns()
      if col_a then
         self:_add_dependency(entry, col_a)
      end
      if col_b then
         self:_add_dependency(entry, col_b)
      end

      -- patch walls depend on roof
      if wc:is_patch_wall() then
         for _, e in pairs(self._sv.structures[ROOF]) do
            self:_add_dependency(entry, e.entity)
         end
      end
   end
end

function Building:_compute_fixture_dependencies()
   for _, entry in pairs(self._sv.structures[FIXTURE_FABRICATOR]) do
      local fixture = entry.entity
      local parent = fixture:get_component('mob')
                                 :get_parent()
                                 
      self:_add_dependency(entry, parent)
   end
   
   self._sv.compute_dependencies_finished = true
   self.__saved_variables:mark_changed()
end

function Building:_compute_inverse_dependencies()
   for _, kind in ipairs(STRUCTURE_TYPES) do
      for id, entry in pairs(self._sv.structures[kind]) do
         assert(entry.dependencies)
         for _, dep in pairs(entry.dependencies) do
            local depe = self:_get_entry_for_structure(dep)
            depe.inverse_dependencies[id] = entry.entity
         end
      end
   end
end

function Building:_get_entry_for_structure(entity)
   local id = entity:get_id()
   for _, kind in ipairs(STRUCTURE_TYPES) do
      local entry = self._sv.structures[kind][id]
      if entry then
         return entry
      end
   end
end

return Building
