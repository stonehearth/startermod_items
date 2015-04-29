local constants = require('constants').construction
local entity_forms = require 'lib.entity_forms.entity_forms_lib'
local build_util = require 'lib.build_util'
local voxel_brush_util = require 'services.server.build.voxel_brush_util'
local BuildUndoManager = require 'services.server.build.build_undo_manager'

local Rect2 = _radiant.csg.Rect2
local Cube3 = _radiant.csg.Cube3
local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3
local Region2 = _radiant.csg.Region2
local Region3 = _radiant.csg.Region3
local Entity = _radiant.om.Entity

local STRUCTURE_COMPONENTS = {
   'stonehearth:wall',
   'stonehearth:floor',
   'stonehearth:column',
   'stonehearth:roof',
}

-- these are quite annoying.  we can get rid of them by implementing and using
-- LuaToProto <-> ProtoToLua in the RPC layer (see lib/typeinfo/dispatcher.h)
local function ToPoint3(pt)
   return pt and Point3(pt.x, pt.y, pt.z) or nil
end

local function ToCube3(box)
   return box and Cube3(ToPoint3(box.min), ToPoint3(box.max)) or nil
end

-- pick which building in a list of blueprints should survive if all the
-- blueprints had to be merged together
--
local function choose_merge_survivor(blueprints)
   local best = nil
   for id, blueprint in pairs(blueprints) do
      if not best or id < best:get_id() then
         best = blueprint
      end
   end
   return build_util.get_building_for(best)
end


local BuildService = class()

function BuildService:__init(datastore)
end

function BuildService:initialize()
   self._undo = BuildUndoManager()

   self.__saved_variables:set_controller(self)

   self._log = radiant.log.create_logger('build.service')
   self._sv = self.__saved_variables:get_data()
   if not self._sv.next_building_id then
      self._sv.next_building_id = 1 -- used to number newly created buildings
      self._sv.ladder_manager = radiant.create_controller('stonehearth:build:ladder_manager')
      self._sv.scaffolding_manager = radiant.create_controller('stonehearth:build:scaffolding_manager')
      self.__saved_variables:mark_changed()
   end
end

function BuildService:_call_all_children(entity, cb)
   local ec = entity:get_component('entity_container')  
   if ec then
      -- Copy the blueprint's (container's) children into a local var first, because
      -- _set_teardown_recursive could cause the entity container to be invalidated.
      local ec_children = {}
      for id, child in ec:each_child() do
         ec_children[id] = child
      end
      for id, child in pairs(ec_children) do
         if child and child:is_valid() then
            self:_call_all_children(child, cb)
         end
      end
   end
   cb(entity)
end

function BuildService:clear_undo_stack()
   self._undo:clear()
end

function BuildService:set_active(entity, enabled)
   if enabled then
      self:clear_undo_stack() -- can't undo once building starts!
      local bc = entity:get_component('stonehearth:building')
      if bc then 
         bc:set_active(true)
      end
   end

   self:_call_all_children(entity, function(entity)
         local c = entity:get_component('stonehearth:construction_progress')
         if c then
            c:set_active(enabled)
         end
      end)
end

function BuildService:set_teardown(entity, enabled)
   if enabled then
      self._undo:clear()      
   end
   self:_call_all_children(entity, function(entity)
         local c = entity:get_component('stonehearth:construction_progress')
         if c then
            c:set_teardown(enabled)
         end
      end)
end

function BuildService:undo_command(session, response)
   self._undo:undo()
   response:resolve(true)
end

-- Add a new floor segment to the world.  This will try to merge with existing buildings
-- if the floor overlaps some pre-existing floor or will create a new building to hold
-- the floor
--
--    @param session - the session for the player initiating the request
--    @param response - a response object which we'll write the result into
--    @param floor_brush - the uri of the brush used to paint the floor
--    @param box - the area of the new floor segment

function BuildService:add_floor_command(session, response, floor_brush, box)
   local floor
   local success = self:do_command('add_floor', response, function()
         floor = self:add_floor(session, floor_brush, ToCube3(box))
      end)

   self:_resolve_and_select_blueprint(success, response, floor)
end

function BuildService:add_road_command(session, response, road_uri, box)
   local road
   local success = self:do_command('add_road', response, function()
         road = self:add_road(session, road_uri, ToCube3(box))
      end)

   self:_resolve_and_select_blueprint(success, response, road)
end

function BuildService:add_wall_command(session, response, column_brush, wall_brush, p0, p1, normal)
   local wall
   local success = self:do_command('add_wall', response, function()
         wall = self:add_wall(session, column_brush, wall_brush, ToPoint3(p0), ToPoint3(p1), ToPoint3(normal))
      end)

   self:_resolve_and_select_blueprint(success, response, wall)
end

function BuildService:repaint_wall_command(session, response, wall, wall_brush)
   local success = self:do_command('repaint_wall', response, function()
         self:repaint_wall(wall, wall_brush)
      end)
   self:_resolve_and_select_blueprint(success, response, wall)
end

function BuildService:repaint_column_command(session, response, blueprint, column_brush)
   local success = self:do_command('repaint_wall', response, function()
         self:repaint_column(blueprint, column_brush)
      end)
   self:_resolve_and_select_blueprint(success, response, blueprint)
end

function BuildService:_resolve_and_select_blueprint(success, response, blueprint)
   if success and blueprint then
      local fab = blueprint:get_component('stonehearth:construction_progress')
                              :get_fabricator_entity()
      response:resolve({
         new_selection = fab
      })
      return
   end
   response:reject({ error = 'something has gone tragicly wrong' })
end


function BuildService:_merge_blueprints(box, acceptable_merge_filter_fn)
   -- look for entities in a 1-voxel border around the box specified.  this lets
   -- us merge adjacent boxes rather than just overlapping ones.
   local overlap = Cube3(Point3(box.min.x - 1, box.min.y - 1, box.min.z - 1),
                         Point3(box.max.x + 1, box.max.y, box.max.z + 1))

   local total_region = Region3(box)

   local all_overlapping_blueprints = radiant.terrain.get_entities_in_cube(overlap, function(entity)
         local is_blueprint = build_util.is_blueprint(entity)

         -- don't merge with things that aren't also blueprints
         local should_merge = is_blueprint

         -- don't merge with other active blueprints!
         if should_merge then
            local cp = entity:get_component('stonehearth:construction_progress')
            assert(cp, 'blueprint has no construction_progress component!')
            should_merge = not cp:get_active()
         end

         -- give the filter a crack at the entity, regardless of whether or not
         -- we've decided to merge.  sometimes the filter has side-effects (e.g. 
         -- updating additional regions!)
         should_merge = acceptable_merge_filter_fn(entity) and should_merge

         if not should_merge then
            -- we're not merging.  too bad! stencil out the opaque shape of whatever it is we can't merge
            -- into
            local rcs = entity:get_component('region_collision_shape')
            if rcs and rcs:get_region_collision_type() ~= _radiant.om.RegionCollisionShape.NONE then
               local region = rcs:get_region():get()
               region = radiant.entities.local_to_world(region, entity)
               total_region:subtract_region(region)
            end
            if is_blueprint then
               local dst = entity:get_component('destination')
               if dst then
                  local region = dst:get_region():get()
                  region = radiant.entities.local_to_world(region, entity)
                  total_region:subtract_region(region)
               end
            end
         end
         return should_merge
      end)
   return all_overlapping_blueprints, total_region
end

function BuildService:add_road(session, road_brush, box)
   return self:_add_floor_type(session, road_brush, box, constants.floor_category.ROAD)
end

function BuildService:add_floor(session, floor_brush, box)
   return self:_add_floor_type(session, floor_brush, box, constants.floor_category.FLOOR)
end

function BuildService:_add_floor_type(session, floor_brush, box, category)
   local all_overlapping, floor_region = self:_merge_blueprints(box, function(entity)
         return build_util.is_blueprint(entity, 'stonehearth:floor') or
                build_util.is_blueprint(entity, 'stonehearth:wall')
      end)

   local parent_building
   if not next(all_overlapping) then
      -- there was no existing floor at all. create a new building and add a floor
      -- segment to it. 
      local building = self:_create_new_building(session, box.min)
      local floor = self:_add_new_floor_to_building(building,
                                                    floor_brush,
                                                    Region3(box),
                                                    category)
      return floor
   end
   -- we overlapped some pre-existing blueprint.  we know none of these blueprints
   -- could previously have been merged, so there are no intersections.  merging
   -- the buildings is easy!  choose one and go for it
   local building = choose_merge_survivor(all_overlapping)
   self:_merge_blueprints_into(building, all_overlapping)

   -- now we have a single `parent_building` which is just missing the `floor_region` floor.
   -- go ahead and add it.

   -- Remove our new region from existing floor.
   local to_merge = self:_subtract_region_from_floor(building,
                                                     category,
                                                     floor_region)
   if radiant.empty(to_merge) then
      local floor = self:_add_new_floor_to_building(building,
                                                    floor_brush,
                                                    Region3(box),
                                                    category)
      return floor
   end
   local merged_floor, merged_cd
   for id, entity in pairs(to_merge) do
      if not merged_cd then
         merged_floor = entity
         merged_cd = merged_floor:get_component('stonehearth:construction_data')
         merged_cd:paint_on_world_region(floor_brush, floor_region)
      else
         local rgn = entity:get_component('destination')
                              :get_region()
                                 :get()
         local origin = radiant.entities.get_world_grid_location(entity)
         radiant.log.write('', 0, 'merging region %s %s', rgn:get_bounds(), rgn:translated(origin))
         merged_cd:add_world_region(rgn:translated(origin))
         self:unlink_entity(entity)
      end
   end

   return merged_floor
end

function BuildService:erase_floor(session, box)
   local floor

   -- look for floor that we can merge into.
   local all_overlapping_floor = radiant.terrain.get_entities_in_cube(box, function(entity)
         local entity_type = self:_get_structure_type(entity)
         return build_util.is_blueprint(entity) and (entity_type == 'floor' or entity_type == 'slab')
      end)

   for _, floor in pairs(all_overlapping_floor) do
      floor:add_component('stonehearth:construction_data')
               :remove_world_region(Region3(box))
   end
end

function BuildService:erase_floor_command(session, response, box)
   local success = self:do_command('erase_floor', response, function()
         self:erase_floor(session, ToCube3(box))
      end)

   return success or nil
end

function BuildService:erase_fixture(fixture_blueprint)
   -- grab the parent before we unlink, since the unlinking process will remove
   -- the entity from the world
   local parent = radiant.entities.get_parent(fixture_blueprint)
   self:unlink_entity(fixture_blueprint)

   -- if we're a portal and were taken off a wall, re-layout to clear the hole.
   local wall = parent:get_component('stonehearth:wall')
   if wall then
      wall:remove_fixture(fixture_blueprint)
               :layout()
   end 
end

function BuildService:erase_fixture_command(session, response, fixture_blueprint)
   local success = self:do_command('erase_fixture', response, function()
         self:erase_fixture(fixture_blueprint)
      end)
   return success or nil
end

-- adds a new fabricator to blueprint.  this creates a new 'stonehearth:build:prototypes:fabricator'
-- entity, adds a fabricator component to it, and wires that fabricator up to the blueprint.
-- See `_init_fabricator` for more details.
--    @param blueprint - The blueprint which needs a new fabricator.
--
function BuildService:add_fabricator(blueprint)
   self._log:info('adding fabricator to %s', blueprint)
   
   assert(blueprint:get_component('stonehearth:construction_data'),
          string.format('blueprint %s has no construction_data', tostring(blueprint)))


   local fabricator = radiant.entities.create_entity('stonehearth:build:prototypes:fabricator', { owner = blueprint })

   local blueprint_mob = blueprint:add_component('mob')
   local parent = blueprint_mob:get_parent()
   if parent then
      parent:add_component('entity_container'):add_child(fabricator)
   end
   local transform = blueprint_mob:get_transform()
   fabricator:add_component('mob')
                  :set_transform(transform)


   local parts = blueprint:get_uri():split(':')
   local desc = parts[#parts]
   fabricator:set_debug_text(desc .. ':' .. tostring(blueprint:get_id()) .. ':fab')
   
   fabricator:add_component('stonehearth:fabricator')
                  :start_project(blueprint)   

   build_util.bind_fabricator_to_blueprint(blueprint, fabricator, 'stonehearth:fabricator')

   return fabricator
end

function BuildService:_bind_building_to_blueprint(building, blueprint)
   -- make the owner of the blueprint the same as the owner as of the building
   blueprint:add_component('unit_info')
            :set_player_id(radiant.entities.get_player_id(building))

   -- fixtures, for example, don't have construction data.  so check first!
   local cd_component = blueprint:get_component('stonehearth:construction_data')
   if cd_component then
      cd_component:set_building_entity(building)
   end

   blueprint:add_component('stonehearth:construction_progress')
               :set_building_entity(building)
end

-- adds a new `blueprint` entity to the specified `building` entity at the optional
-- location.  also handles making sure the blueprint is owned by the building owner
-- and creating a fabricator for the blueprint
--
--    @param building - The building entity which will contain the blueprint
--    @param blueprint - The blueprint to be added to the building
--    @param offset - a Point3 representing the offset in the building
--                      where the blueprint is located
--
function BuildService:_create_blueprint(building, blueprint_uri, offset, init_fn)
   local blueprint = radiant.entities.create_entity(blueprint_uri, { owner = building })

   self:_bind_building_to_blueprint(building, blueprint)
   blueprint:set_debug_text('blueprint')

   -- give it a region and stick it in the building...
   blueprint:add_component('destination')
               :set_region(radiant.alloc_region3())

   -- if an offset is specified, stick the object in the building.  if not,
   -- the init function must take care of it
   if offset then
      radiant.entities.add_child(building, blueprint, offset)
   end

   -- initialize...
   init_fn(blueprint)

   -- add a fabricator to the blueprint now that it's initialized.
   local fabricator = self:add_fabricator(blueprint)

   return blueprint, fabricator
end

-- creates a new building for the owner of `sesssion` located at `origin` in
-- the world
--
--     @param session - the session of the owning player
--     @param location - a Point3 representing the position of the new building in the
--                       world
--
function BuildService:_create_new_building(session, location)
   local building = radiant.entities.create_entity('stonehearth:build:prototypes:building', { owner = session.player_id })
   self._undo:trace_building(building)
   
   -- give the building a unique name and establish ownership.
   building:add_component('unit_info')
           :set_display_name(string.format('Building No.%d', self._sv.next_building_id))
           :set_player_id(session.player_id)
           

   self._sv.next_building_id = self._sv.next_building_id + 1
   self.__saved_variables:mark_changed()

   -- all buildings have a building component to manage the interaction between
   -- individual structures
   building:add_component('stonehearth:building')

   -- add a construction progress component.  though the building is just a container
   -- and requires no fabricator, we still need a CP component to activate and track
   -- progress.
   building:add_component('stonehearth:construction_progress')

   -- finally, put the entity on the ground at the requested location
   radiant.terrain.place_entity(building, location)

   return building
end

function BuildService:_subtract_region_from_floor(building_ent, category, region)
   local to_merge = {}

   local building = building_ent:get_component('stonehearth:building')
   local structures = building:get_all_structures()
   for id, entry in pairs(structures['stonehearth:floor']) do
      local entity = entry.entity
      local origin = radiant.entities.get_world_grid_location(entity)
      local rgn = entity:get_component('destination')
                           :get_region()
                              :get()
                                 :translated(origin)
                                    
      local collides = rgn:inflated(Point3.one)
                              :intersects_region(region)

      if collides then         
         entity:get_component('stonehearth:construction_data')
                  :remove_world_region(region)
         if entry.structure:get_category() == category then
            to_merge[id] = entity
         end
      end
   end

   return to_merge
end

-- Add a new floor to an existing building.  You almost certainly don't want to call this directly;
-- callers need to ensure that the new floor doesn't overlap with any other floors.  Look at
-- _merge_floor_into_building for more context.  add_floor and add_road both do the required
-- merging and clipping.
--    @param building - the building to contain the new floor
--    @param floor_brush - the uri of the brush used to paint the floor
--    @param floor_region - the area of the new floor segment
--    @param floor_type - the type of the floor (road, floor)
--
function BuildService:_add_new_floor_to_building(building, floor_brush, floor_region, floor_type)
   -- try very hard to make the local region in the floor sane.  this means translating the
   -- floor entity!
   local bounds = floor_region:get_bounds()
   local origin = radiant.entities.get_world_grid_location(building)
   local local_origin = bounds.min - origin

   local prototype = (floor_type == constants.floor_category.FLOOR) and 'floor' or 'road'
   
   local floor_ent = self:_create_blueprint(building, 'stonehearth:build:prototypes:' .. prototype, local_origin, function(floor)
         floor:add_component('stonehearth:construction_data')
                  :paint_on_world_region(floor_brush, floor_region)
      end)
               
   floor_ent:get_component('stonehearth:floor')
               :set_category(floor_type) -- xxx: must be called after the floor has a fabricator for roads.  ug!

   return floor_ent
end

-- return the type of the specified structure (be it a blueprint or a project)
-- or nil of the entity is not a structure.  structures have 
-- 'stonehearth:construction_data' components
--
--    @param entity - the entity to be tested for blueprintedness
--
function BuildService:_get_structure_type(entity)
   local cd = entity:get_component('stonehearth:construction_data')
   return cd and cd:get_type() or nil
end

-- merge a set of buildings together into the `merge_into` building, destroying
-- the now empty husks left in `buildings_to_merge`.
--
--    @param merge_into - a building entith which will contain all the
--                        children of all the buildings in `buildings_to_merge`
--    @param buildings_to_merge - a table of buildings which should be merged
--                                into `merge_into`, then destroyed.
--
function BuildService:_merge_buildings_into(merge_into, buildings_to_merge)
   for _, building in pairs(buildings_to_merge) do
      if building ~= merge_into then
         self:_merge_building_into(merge_into, building)
      end
   end
end


function BuildService:_merge_blueprints_into(merge_into, blueprints)
   local buildings = {}
   for _, blueprint in pairs(blueprints) do
      local building = build_util.get_building_for(blueprint)
      buildings[building:get_id()] = building
   end
   self:_merge_buildings_into(merge_into, buildings)
end


-- merge all the children of `building` into `merge_into` and destroy
-- `building`
--
--    @param merge_into - a building entity which will contain all the
--                        children of all the buildings in `building`
--    @param building - the building which should be merged into `merge_into`,
--                      then destroyed.
--
function BuildService:_merge_building_into(merge_into, building)
   local building_offset = radiant.entities.get_world_grid_location(building) - 
                           radiant.entities.get_world_grid_location(merge_into)

   local container = merge_into:get_component('entity_container')

   -- Only move the blueprint!  Regenerate a new project/fabricator for the moved
   -- blueprint.
   for id, child in building:get_component('entity_container'):each_child() do
      if build_util.is_blueprint(child) then
         local mob = child:get_component('mob')
         local child_offset = mob:get_grid_location()

         container:add_child(child)
         mob:set_location_grid_aligned(child_offset + building_offset)

         self:_bind_building_to_blueprint(merge_into, child)

         -- Re-generate fabricators (which in turn re-generate projects.)
         local fixture_fab = child:get_component('stonehearth:fixture_fabricator')
         if fixture_fab then
            self:add_fixture_fabricator(child, fixture_fab:get_uri(), fixture_fab:get_normal(), fixture_fab:get_rotation())
         else
            self:add_fabricator(child)
         end
      end
   end

   self:unlink_entity(building)
end

function BuildService:repaint_wall(wall, wall_brush)
   local wc = wall:get_component('stonehearth:wall')
   if not wc then
      return
   end

   wc:set_brush(wall_brush)
   wc:layout()

   return wall
end


function BuildService:repaint_column(blueprint, column_brush)   
   local cc = blueprint:get_component('stonehearth:column')
   if cc then
      cc:set_brush(column_brush)
      cc:layout()
      return
   end

   local wc = blueprint:get_component('stonehearth:wall')
   if wc then
      local columns = { wc:get_columns() }
      for _, column in pairs(columns) do
         self:repaint_column(column, column_brush)
      end
   end
end

function BuildService:add_wall(session, column_brush, wall_brush, p0, p1, normal)
   -- look for floor that we can merge into.
   local c0 = self:_get_blueprint_at_point(p0)
   local c1 = self:_get_blueprint_at_point(p1)
   local either_column = c0 or c1
   local building

   if c0 and c1 then
      -- connecting two existing columns.  
      local b0 = build_util.get_building_for(c0)
      local b1 = build_util.get_building_for(c1)
      assert(b0 == b1) -- merge required
      building = b0
   elseif either_column then
      -- connecting to just one column.  that's ok too!
      building = build_util.get_building_for(either_column)
   else
      -- hm.  brand new wall.  see if we can find another building near it.  for
      -- now, that means just below (e.g. if we're stacking walls)
      local box = _radiant.csg.construct_cube3(p0 - Point3.unit_y, p1 + normal, 0)
      local all_overlapping = radiant.terrain.get_entities_in_cube(box, function(entity)
            return build_util.is_blueprint(entity)
         end)
      if next(all_overlapping) then
         -- yay!  merge all this stuff together and go!
         building = choose_merge_survivor(all_overlapping)
         self:_merge_blueprints_into(building, all_overlapping)         
      end
   end
   if not building then
      -- all our efforts have failed!  make a brand new building for this wall.
      building = self:_create_new_building(session, p0)
   end

   return self:_add_wall_span(building, p0, p1, normal, column_brush, wall_brush)
end

-- add walls around all the floor segments for the specified `building` which
-- do not already have walls around them.
--
--    @param session - the session for the player initiating the request
--    @param response - a response object which we'll write the result into
--    @param floor - the floor we need to put walls around
--    @param column_brush - the type of columns to generate
--    @param wall_brush - the type of walls to generate
--
function BuildService:grow_walls_command(session, response, floor, column_brush, wall_brush)
   local success = self:do_command('grow_walls', response, function()
         self:grow_walls(floor, column_brush, wall_brush)
      end)
   return success or nil
end

function BuildService:grow_walls(floor, column_brush, wall_brush)
   local building = build_util.get_building_for(floor)
   local walls = {}

   -- sometimes floor is a fabricator
   floor = build_util.get_blueprint_for(floor)
   if not floor then
      return
   end
   local fc = floor:get_component('stonehearth:floor')
   if not fc then
      return
   end

   build_util.grow_walls_around(floor, function(min, max, normal)
         local wall = self:_add_wall_span(building, min, max, normal, column_brush, wall_brush)
         walls[wall:get_id()] = wall

         fc:connect_to(wall)
         local columns = { wall:get_component('stonehearth:wall')
                                 :get_columns() }
         for _, column in pairs(columns) do
            fc:connect_to(column)
         end
      end)
   return walls
end


-- Pops a roof on a building with no roof. 
--
--    @param session - the session for the player initiating the request
--    @param response - a response object which we'll write the result into
--    @param building - the building to pop the roof onto

function BuildService:grow_roof_command(session, response, root_wall, roof_options)
   local roof
   local success = self:do_command('grow_roof', response, function()
         roof = self:grow_roof(root_wall, roof_options)
      end)

   self:_resolve_and_select_blueprint(success, response, roof)
end

function BuildService:grow_roof(root_wall, roof_options)
   checks('self', 'Entity', 'table')
   local building = build_util.get_building_for(root_wall)

   -- compute the roof shape
   local world_origin, region2, walls = build_util.calculate_roof_shape_around_walls(root_wall, roof_options)

   -- make sure we can grow a roof here.  firstly, if any of the walls in the loop
   -- already have a roof, bail.
   local success, result1 = build_util.can_grow_roof_around_walls(walls)
   if not success then
      return
   end
   local columns = result1

   -- create the roof.
   local origin = world_origin - radiant.entities.get_world_grid_location(building)
   local roof = self:_create_blueprint(building, 'stonehearth:build:prototypes:roof', origin, function(roof_entity)
         roof_entity:add_component('stonehearth:roof')
                        :apply_nine_grid_options(roof_options)
                        :cover_region2(roof_options.brush, region2)
      end)

   -- connect all the walls and columns to the roof...
   for _, wall in pairs(walls) do
      wall:get_component('stonehearth:wall')
               :connect_to_roof(roof)
   end
   for _, column in pairs(columns) do
      column:get_component('stonehearth:column')
               :connect_to_roof(roof)
   end

   -- layout to make sure the modified walls and columns reach the roof
   building:get_component('stonehearth:building')
               :layout_roof(roof)

   return roof
end

-- returns the blueprint at the specified world `point`
--    @param point - the world space coordinate of where you're looking
--
function BuildService:_get_blueprint_at_point(point)
   local entities = radiant.terrain.get_entities_at_point(point, function(entity)
         return build_util.is_blueprint(entity)
      end)

   local id, blueprint = next(entities)
   if blueprint then
      local _, overlapped = next(entities, id)
      assert(not overlapped)
   end
   
   return blueprint
end

-- returns the blueprint at the specified world `point`, or creates one if there isn't
-- one there.  generally speaking, you should not be using this function.  use one of
-- the helpers (e.g. :_fetch_column_at_point)
--    @param building - the building the structure  should be in (or created in)
--    @param point - the world space coordinate of where you're looking
--    @param blueprint_uri - the type of structure to create if there's nothing at `point`
--    @param init_fn - a callback used to initiaize the new structure if it needs to be
--                     created
--
function BuildService:_fetch_blueprint_at_point(building, point, blueprint_uri, init_fn)
   local blueprint = self:_get_blueprint_at_point(point)
   if blueprint then
      assert(build_util.get_building_for(blueprint) == building)
   else
      local origin = radiant.entities.get_world_grid_location(building)
      blueprint = self:_create_blueprint(building, blueprint_uri, point - origin, init_fn)
   end
   return blueprint
end

-- returns the column at the specified world `point`, or creates one if there is no
-- column there.
--    @param building - the building the column should be in (or created in)
--    @param point - the world space coordinate of where you're looking
--    @param column_brush - the type of column to create if there's nothing at `point`
--
function BuildService:_fetch_column_at_point(building, point, column_brush)
   return self:_fetch_blueprint_at_point(building, point, 'stonehearth:build:prototypes:column', function(column)
         column:add_component('stonehearth:column')
                  :set_brush(column_brush)
                  :layout()
      end)
end

-- adds a span of walls between `min` and `max`, creating columns and
-- wall segments as required `min` must be less than `max` and the
-- requests span must be grid aligned (no diagonals!)
--    @param building - the building to put all the new objects in
--    @param min - the start of the wall span. 
--    @param max - the end of the wall span.
--    @param column_brush - the type of columns to generate
--    @param wall_brush - the type of walls to generate
--
function BuildService:_add_wall_span(building, min, max, normal, column_brush, wall_brush)
   local col_a = self:_fetch_column_at_point(building, min, column_brush)
   local col_b = self:_fetch_column_at_point(building, max, column_brush)
   if col_a and col_b then
      return self:_create_wall(building, col_a, col_b, normal, wall_brush)
   end
end

-- creates a wall inside `building` connecting `column_a` and `column_b`
-- pointing in the direction of `normal`.  
--    @param building - the building to add the wall to
--    @param column_a - one of the columns to attach the wall to
--    @param column_b - the other column to attach the wall to
--    @param normal - a Point3 pointing "outside" of the wall
--    @param wall_brush - the type of wall to build
--
function BuildService:_create_wall(building, column_a, column_b, normal, wall_brush)
   return self:_create_blueprint(building, 'stonehearth:build:prototypes:wall', Point3.zero, function(wall)      
         wall:add_component('stonehearth:wall')
                  :connect_to_columns(column_a, column_b, normal)
                  :set_brush(wall_brush)
                  :layout()

         wall:add_component('stonehearth:construction_data')
                  :set_normal(normal)
      end)
end


-- creates a fixture inside `parent_entity` at `location`  
--    @param parent_entity - the structure you'd like to contain the fixture
--    @param fixture_uri - the type of fixture to create
--    @param location - where to put the fixture, in wall-local coordinates
--
function BuildService:add_fixture_command(session, response, parent_entity, fixture_uri, location, normal, rotation)
   local fixture
   local success = self:do_command('add_fixture', response, function()
         fixture = self:add_fixture(parent_entity, fixture_uri, ToPoint3(location), ToPoint3(normal), rotation)
      end)

   if success then
      response:resolve({
         new_selection = fixture
      })
      return
   end
   response:reject({ error = 'failed to add fixture'})
end

function BuildService:add_fixture_fabricator(fixture_blueprint, fixture_or_uri, normal, rotation, always_show_ghost)
   -- use ghost material if explicitly placed by user and blueprint material if placed by the template
   local material = always_show_ghost and 'materials/ghost_item.json' or 'materials/blueprint.material.json'

   -- `fixture` is actually a blueprint of the fixture, not the actual fixture
   -- itself.  change the material we use to render it and hook up a fixture_fabricator
   -- to help build it.
   fixture_blueprint:add_component('render_info'):set_material(material)
   fixture_blueprint:add_component('stonehearth:ghost_form')
   
   local fab_component = fixture_blueprint:add_component('stonehearth:fixture_fabricator')
   fab_component:set_always_show_ghost(always_show_ghost)
   fab_component:start_project(fixture_or_uri, normal, rotation)

   build_util.bind_fabricator_to_blueprint(fixture_blueprint, fixture_blueprint, 'stonehearth:fixture_fabricator')

   return fab_component
end

function BuildService:add_fixture(parent_entity, fixture_or_uri, location, normal, rotation, always_show_ghost)
   if always_show_ghost == nil then
      always_show_ghost = false
   end

   if not build_util.is_blueprint(parent_entity) then
      self._log:info('cannot place fixture %s on non-blueprint entity %s', fixture_or_uri, parent_entity)
      return
   end

   if not normal then
      normal = parent_entity:get_component('stonehearth:construction_data')
                              :get_normal()
   end
   assert(normal)
   
   local _, fixture_blueprint

   local _, _, fixture_ghost_uri = entity_forms.get_uris(fixture_or_uri)
   fixture_blueprint = radiant.entities.create_entity(fixture_ghost_uri, {
         owner = parent_entity,
         debug_text = 'fixture blueprint',
      })

   local building = build_util.get_building_for(parent_entity)

   self:_bind_building_to_blueprint(building, fixture_blueprint)

   local wall = parent_entity:get_component('stonehearth:wall')
   if wall then
      -- add the new fixture to the wall and reconstruct the shape.
      wall:add_fixture(fixture_blueprint, location, normal)
               :layout()
   else
      radiant.entities.add_child(parent_entity, fixture_blueprint, location)
      radiant.entities.turn_to(fixture_blueprint, rotation or 0)
   end

   self:add_fixture_fabricator(fixture_blueprint, fixture_or_uri, normal, rotation, always_show_ghost)

   -- fixtures can be added to the building after it's already been started.
   -- if this is the case, go ahead and start the placing process
   local active = building:get_component('stonehearth:construction_progress')
                              :get_active()
   if active then
      fixture_blueprint:get_component('stonehearth:construction_progress')
                           :set_active(true)
   end
   return fixture_blueprint
end

function BuildService:_substitute_blueprint(old, new_uri)
   -- make sure the old building has a parent.  if it doesn't, it means the structure had
   -- been unlinked, and the client somehow sent us some state data (perhaps as a race)
   if not old:add_component('mob'):get_parent() then
      return
   end
   
   local building = build_util.get_building_for(old)   
   local replaced = self:_create_blueprint(building, new_uri, Point3.zero, function(new)
         -- place the new entity in the same position as the old one
         local mob = old:get_component('mob')
         new:add_component('mob')
               :set_transform(mob:get_transform())

         -- move all the children of `old` into `new`
         local old_ec = old:get_component('entity_container')
         if old_ec then
            local new_ec = new:add_component('entity_container')
            for _, child in old_ec:each_child() do
               new_ec:add_child(child)
            end
         end

         -- clone all the components that pertain to structure.
         new:add_component('stonehearth:construction_data')
               :clone_from(old)

         for _, name in ipairs(STRUCTURE_COMPONENTS) do
            local old_structure = old:get_component(name)
            if old_structure then
               new:add_component(name)
                           :clone_from(old)
            end
         end
      end)

   -- nuke the old entity
   self:unlink_entity(old)

   return replaced
end

function BuildService:unlink_entity(entity)
   local building = build_util.get_building_for(entity)
   if building then
      building:get_component('stonehearth:building')
                  :unlink_entity(entity)
   end
   self._undo:unlink_entity(entity)
end

-- called by a remote ui to change properties in the construction data component.
-- 
--    @param session - the session for the user at the other end of the connectino
--    @param response - the response object
--    @param options - the options to change.  
--
function BuildService:apply_options_command(session, response, blueprint, options)
   local success = self:do_command('apply_options', response, function()
         self:_apply_nine_grid_options(blueprint, options)
      end)
   return success or nil
end

function BuildService:_apply_nine_grid_options(blueprint, options)
   local roof = blueprint:get_component('stonehearth:roof')
   if roof then
      -- apply the new options to the blueprint      
      roof:apply_nine_grid_options(options)

      local building = build_util.get_building_for(blueprint)
      building:get_component('stonehearth:building')
                  :layout_roof(blueprint)

      --[[
      -- copy the options to the project.
      local fab = blueprint:get_component('stonehearth:construction_data')
                              :get_fabricator_entity()
      local proj = fab:get_component('stonehearth:fabricator')
                           :get_project()
      proj:get_component('stonehearth:construction_data')
               :clone_from(blueprint)
      ]]
      return true
   end
   return false
end

-- called by the roof to add a patch wall to the specified building. 
--
--    @param building - the building which needs a new patch wall.
--    @param wall_brush - the type of wall to create
--    @param normal - the normal for the wall
--    @param position - the local coordinate of the wall to add
--    @param region - the exact shape of the patch wall
--
function BuildService:add_patch_wall_to_building(building, wall_brush, normal, position, region)
   self:_create_blueprint(building, 'stonehearth:build:prototypes:patch_wall', position, function(wall)
         wall:add_component('stonehearth:wall')
                  :set_brush(wall_brush)
                  :create_patch_wall(normal, region)
                  :layout()
      end)
end

function BuildService:create_ladder_command(session, response, ladder_uri, location, normal)
   normal = ToPoint3(normal)
   location = ToPoint3(location)
   self._sv.ladder_manager:request_ladder_to(session.player_id, location, normal, true)
   return true
end

function BuildService:remove_ladder_command(session, response, ladder_entity)
   if radiant.util.is_a(ladder_entity, Entity) then
      local ladder = ladder_entity:get_component('stonehearth:ladder')
      if ladder then
         local base = radiant.entities.get_world_grid_location(ladder_entity)
         self._sv.ladder_manager:remove_ladder(base)
      end
   end
   return true
end

function BuildService:request_ladder_to(owner, climb_to, normal)
   return self._sv.ladder_manager:request_ladder_to(owner, climb_to, normal)
end

function BuildService:request_scaffolding_for(owner, blueprint_rgn, project_rgn, normal, stand_at_base)
   return self._sv.scaffolding_manager:request_scaffolding_for(owner, blueprint_rgn, project_rgn, normal, stand_at_base)
end

function BuildService:instabuild_command(session, response, building)
   -- get everything built.
   self:instabuild(building)
   return true
end

function BuildService:instabuild(building)
   building:get_component('stonehearth:building')
               :set_active(true)

   self:_call_all_children(building, function(entity)
         local cp = entity:get_component('stonehearth:construction_progress')
         if cp and entity:get_uri() ~= 'stonehearth:build:prototypes:scaffolding' then
            local fabricator = cp:get_fabricator_component()
            if fabricator then
               self._log:info( '  fabricator %s -> %s instabuild', entity:get_uri(), fabricator._entity)
               fabricator:instabuild()
            end
            -- mark as active.  this is a nop now that we've built it, but the
            -- bit still need be set to mimic the building actually being built
            cp:set_active(true)
         end
      end)
end

function BuildService:get_cost_command(session, response, building)
   return build_util.get_cost(building)
end

function BuildService:do_command(reason, response, cb)
   self._undo:begin_transaction(reason)
   self._in_transaction = true
   self._at_end_of_transaction_cbs = {}
   cb()
   self._in_transaction = false
   
   local _, cb = next(self._at_end_of_transaction_cbs)
   while cb do
      cb()
      _, cb = next(self._at_end_of_transaction_cbs, _)
   end
   self._at_end_of_transaction_cbs = {}

   self._undo:end_transaction(reason)
   return true
end

function BuildService:in_transaction()
   return self._in_transaction
end

function BuildService:at_end_of_transaction(fn)
   table.insert(self._at_end_of_transaction_cbs, fn)
end

function BuildService:build_template_command(session, response, template_name, location, centroid, rotation)
   local building
   self:do_command('build_template', response, function()
         building  = self:build_template(session, template_name, ToPoint3(location), ToPoint3(centroid), rotation)
      end)
   return { building = building }
end

function BuildService:build_template(session, template_name, location, centroid, rotation)
   local position = location - centroid:rotated(rotation)
   local building = self:_create_new_building(session, position)
   
   build_util.restore_template(building, template_name, {
         rotation = rotation
      })

   return building
end

function BuildService:get_scaffolding_manager_command(session, response)
   return {
      scaffolding_manager = self._sv.scaffolding_manager
   }
end

return BuildService

