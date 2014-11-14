local constants = require('constants').construction
local entity_forms = require 'lib.entity_forms.entity_forms_lib'
local build_util = require 'lib.build_util'
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
      self._sv.scaffolding_manager = radiant.create_controller('stonehearth:build_scaffolding_manager')
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
--    @param floor_uri - the uri to type of floor we'd like to add
--    @param box - the area of the new floor segment

function BuildService:add_floor_command(session, response, floor_uri, box, brush_shape)
   local floor
   local success = self:do_command('add_floor', response, function()
         floor = self:add_floor(session, floor_uri, ToCube3(box), brush_shape)
      end)

   if success then
      -- if we managed to create some floor, return the fabricator to the client as the
      -- new selected entity.  otherwise, return an error.
      if floor then
         local floor_fab = floor:get_component('stonehearth:construction_progress'):get_fabricator_entity()
         response:resolve({
            new_selection = floor_fab
         })
      else
         response:reject({ error = 'could not create floor' })
      end
   end
end

function BuildService:add_road_command(session, response, road_uri, box, brush_shape)
   local road
   local success = self:do_command('add_road', response, function()
         road = self:add_road(session, road_uri, ToCube3(box), brush_shape)
      end)

   if success then
      -- if we managed to create some road, return the fabricator to the client as the
      -- new selected entity.  otherwise, return an error.
      if road then
         local road_fab = road:get_component('stonehearth:construction_progress'):get_fabricator_entity()
         response:resolve({
            new_selection = road_fab
         })
      else
         response:reject({ error = 'could not create road' })
      end
   end
end

function BuildService:add_wall_command(session, response, columns_uri, walls_uri, p0, p1, normal)
   local wall
   local success = self:do_command('add_wall', response, function()
         wall = self:add_wall(session, columns_uri, walls_uri, ToPoint3(p0), ToPoint3(p1), ToPoint3(normal))
      end)

   if success then
      if wall then
         local wall_fab = wall:get_component('stonehearth:construction_progress'):get_fabricator_entity()      
         response:resolve({
            new_selection = wall_fab
         })
      else
         response:reject({ error = 'could not create wall' })      
      end
   end
end

-- Given a Region2 total_region, convert that into a road region, return both the curb
-- and the road.
function BuildService:_region_to_road_regions(total_region, origin)
   local proj_curb_region = Region2()
   local road_region = Region3()
   local curb_region = nil
   if total_region:get_bounds():width() >= 3 and total_region:get_bounds():height() >= 3 then
      local edges = total_region:get_edge_list()
      curb_region = Region3()
      for edge in edges:each_edge() do
         local min = Point2(edge.min.location.x, edge.min.location.y)
         local max = Point2(edge.max.location.x, edge.max.location.y)

         if min.y == max.y then
            if edge.min.accumulated_normals.y < 0 then
               max.y = max.y + 1
            elseif edge.min.accumulated_normals.y > 0 then
               min.y = min.y - 1
            end
         elseif min.x == max.x then
            if edge.min.accumulated_normals.x < 0 then
               max.x = max.x + 1
            elseif edge.min.accumulated_normals.x > 0 then
               min.x = min.x - 1
            end
         end

         local c = Cube3(Point3(min.x, origin.y, min.y), Point3(max.x, 2 + origin.y, max.y))
         curb_region:add_cube(c)
      end

      proj_curb_region = curb_region:project_onto_xz_plane()
   end

   local proj_road_region = total_region - proj_curb_region

   for cube in proj_road_region:each_cube() do
      local c = Cube3(Point3(cube.min.x, origin.y, cube.min.y),
         Point3(cube.max.x, origin.y + 1, cube.max.y))
      road_region:add_cube(c)
   end

   return curb_region, road_region
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

function BuildService:add_road(session, road_uri, box, brush_shape, curb_uri, curb_height)
   local road = nil
   local origin = box.min
   local clip_against_curbs = Region3()

   -- look for road that we can merge into.
   local all_overlapping_road, total_region = self:_merge_blueprints(box, function(entity)
         -- it's odd to have curbs around the edges of roads that lead right up to doors,
         -- so take those off automatically
         if entity:get_component('stonehearth:portal') then
            local rcs = entity:get_component('region_collision_shape')
            if rcs then
               local r = radiant.entities.local_to_world(rcs:get_region():get(), entity)
               clip_against_curbs:add_region(r)
            end
            return false
         end

         -- Only merge with other road.
         local fc = entity:get_component('stonehearth:floor')
         return fc and fc:get_category() == constants.floor_category.ROAD
      end)

   local proj_total_region = total_region:project_onto_xz_plane()

   if not next(all_overlapping_road) then
      local building = self:_create_new_building(session, origin)
      local curb_region, road_region = self:_region_to_road_regions(proj_total_region, origin)

      if curb_region then
         curb_region:subtract_region(clip_against_curbs)
         -- We might not have a curb (road was too narrow!)
         self:_add_new_road_to_building(building, road_uri, curb_region, brush_shape)
      end
      road = self:_add_new_road_to_building(building, road_uri, road_region, brush_shape)
   else
      -- we overlapped some pre-existing floor.  merge this box into that floor,
      -- potentially merging multiple buildings together!
      road = self:_merge_overlapping_roads(all_overlapping_road, road_uri, proj_total_region, brush_shape, origin, clip_against_curbs)
   end
   return road
end

function BuildService:add_floor(session, floor_uri, box, brush_shape)
   local all_overlapping_floor, floor_region = self:_merge_blueprints(box, function(entity)
         -- Only merge with floors.
         local fc = entity:get_component('stonehearth:floor')
         return fc and fc:get_category() == constants.floor_category.FLOOR
      end)

   local floor
   if not next(all_overlapping_floor) then
      -- there was no existing floor at all. create a new building and add a floor
      -- segment to it. 
      local building = self:_create_new_building(session, box.min)
      floor = self:_add_new_floor_to_building(building, floor_uri, floor_region, brush_shape)
   else
      -- we overlapped some pre-existing floor.  merge this box into that floor,
      -- potentially merging multiple buildings together!
      floor = self:_merge_overlapping_floor(all_overlapping_floor, floor_uri, floor_region, brush_shape)
   end

   return floor   
end

function BuildService:erase_floor(session, box)
   local floor

   -- look for floor that we can merge into.
   local all_overlapping_floor = radiant.terrain.get_entities_in_cube(box, function(entity)
         return build_util.is_blueprint(entity) and self:_get_structure_type(entity) == 'floor'
      end)

   for _, floor in pairs(all_overlapping_floor) do
      floor:add_component('stonehearth:floor')
               :remove_region_from_floor(Region3(box))
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

-- adds a new fabricator to blueprint.  this creates a new 'stonehearth:entities:fabricator'
-- entity, adds a fabricator component to it, and wires that fabricator up to the blueprint.
-- See `_init_fabricator` for more details.
--    @param blueprint - The blueprint which needs a new fabricator.
--
function BuildService:add_fabricator(blueprint)
   assert(blueprint:get_component('stonehearth:construction_data'),
          string.format('blueprint %s has no construction_data', tostring(blueprint)))

   local fabricator = radiant.entities.create_entity('stonehearth:entities:fabricator')

   local blueprint_mob = blueprint:add_component('mob')
   local parent = blueprint_mob:get_parent()
   if parent then
      parent:add_component('entity_container'):add_child(fabricator)
   end
   local transform = blueprint_mob:get_transform()
   fabricator:add_component('mob')
                  :set_transform(transform)

   fabricator:set_debug_text('(Fabricator for ' .. tostring(blueprint) .. ')')
   
   fabricator:add_component('stonehearth:fabricator')
                  :start_project(blueprint)   

   self:_bind_fabricator_to_blueprint(blueprint, fabricator, 'stonehearth:fabricator')

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

function BuildService:_bind_fabricator_to_blueprint(blueprint, fabricator, fabricator_component_name)
   local fabricator_component = fabricator:get_component(fabricator_component_name)
   assert(fabricator_component)
   
   local building = build_util.get_building_for(blueprint)
   local project = fabricator_component:get_project()
   if project then
      local cd_component = project:get_component('stonehearth:construction_data')
      if cd_component then
         cd_component:set_building_entity(building)
                     :set_fabricator_entity(fabricator)
      end
   end
   blueprint:get_component('stonehearth:construction_progress')
               :set_fabricator_entity(fabricator, fabricator_component_name)
               
   -- fixtures, for example, don't have construction data.  so check first!
   local cd_component = blueprint:get_component('stonehearth:construction_data')
   if cd_component then
      cd_component:set_fabricator_entity(fabricator)
   end

   -- track the structure in the building component.  the building component is
   -- responsible for cross-structure interactions (e.g. sharing scaffolding)
   building:get_component('stonehearth:building')
               :add_structure(blueprint)
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
   local blueprint = radiant.entities.create_entity(blueprint_uri)

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
   local building = radiant.entities.create_entity('stonehearth:entities:building')
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

-- adds a new piece of floor to some existing set of floor objects contained in the
-- `existing_floor` map (keyed by entity id).  if the `box` overlaps many different
-- pieces of floor, we'll have to merge them all together into a single floor entity.
-- if those pieces of floor belong to different buildings, merge all the buildings
-- together, too!
--
--    @param existing_floor - a table of all floor segments that overlap the `box`
--    @param floor_uri - the uri to type of floor we'd like to add
--    @param box - the area of the new floor segment
--
function BuildService:_merge_overlapping_floor(existing_floor, floor_uri, new_floor_region, brush_shape)
   local id, floor = next(existing_floor)
   local id, next_floor = next(existing_floor, id)

   self:_merge_overlapping_floor_trivial(floor, new_floor_region, brush_shape)
   if not next_floor then
      return floor
   end

   -- gah!  this is going to be tricky.  first, get all the buildings that
   -- own all these pieces of floor and merge them together.  choose the floor
   -- that was created earliest to merge into
   floor = nil
   for id, f in pairs(existing_floor) do
      if not floor or id < floor:get_id() then
         floor = f
      end
   end

   -- merge all the floors into the single floor we picked out earlier.
   for _, old_floor in pairs(existing_floor) do
      if old_floor ~= floor then
         local floor_origin = radiant.entities.get_world_location(old_floor)
         self:_merge_overlapping_floor_trivial(floor, old_floor:get_component('destination'):get_region():get():translated(floor_origin), brush_shape)
      end
   end

   -- now unlink all the old buildings.
   local buildings_to_merge = {}
   local floor_building = build_util.get_building_for(floor)
   for _, old_floor in pairs(existing_floor) do
      local building = build_util.get_building_for(old_floor)
      if building ~= floor_building then
         self:unlink_entity(building)
         self:unlink_entity(old_floor)
      end
   end

   return floor
end

function BuildService:_merge_overlapping_roads(existing_roads, road_uri, new_road_region, brush_shape, origin, clip_against_curbs)
   -- Select a road to merge into.
   local road = nil
   for id, f in pairs(existing_roads) do
      if not road or id < road:get_id() then
         road = f
      end
   end

   -- Find and merge the buildings for those roads.
   local buildings_to_merge = {}
   local road_building = build_util.get_building_for(road)
   for _, old_road in pairs(existing_roads) do
      local building = build_util.get_building_for(old_road)
      if building ~= road_building then
         buildings_to_merge[building:get_id()] = building
      end
   end
   self:_merge_buildings_into(road_building, buildings_to_merge)

   -- Actually merge those roads.
   for _, old_road in pairs(existing_roads) do
      if old_road ~= road then
         road:get_component('stonehearth:floor'):merge_with(old_road)
      end
   end

   -- Okay, finally the new guy.  Generate a road and curb region.
   local curb_region, road_region = self:_region_to_road_regions(new_road_region, origin)

   -- Build a new region that is the road region, extruded up a bit (enough to encompass
   -- any existing curb).
   local extruded_road = Region3()
   for cube in road_region:each_cube() do
      local c = Cube3(cube.min, Point3(cube.max.x, cube.max.y + 2, cube.max.z))
      extruded_road:add_cube(c)
   end

   -- Subtracting the extruded region removes all that area from the existing roads, thus
   -- making room for the new road.
   road:get_component('stonehearth:floor'):remove_region_from_floor(extruded_road)

   -- Add the road.
   road:get_component('stonehearth:floor'):add_region_to_floor(road_region, brush_shape)

   if curb_region then
      -- The curb is the curb we generated, removing anything that overlaps with existing
      -- road.
      -- Again, make an extruded region, this time from all the existing roads, extruded up
      -- a bit, so that we can eliminate any crossover with the new curb (new curb should NOT
      -- be built on existing road!)
      local extruded_road = Region3()
      for cube in road:get_component('stonehearth:floor'):get_region():get():each_cube() do
         local c = Cube3(cube.min, Point3(cube.max.x, cube.max.y + 2, cube.max.z))
         extruded_road:add_cube(c)
      end
      extruded_road:translate(radiant.entities.get_world_grid_location(road))

      -- Eliminate those bits from the generated curb.
      curb_region:subtract_region(extruded_road)
      curb_region:subtract_region(clip_against_curbs)

      -- Add the curb.
      road:add_component('stonehearth:floor'):add_region_to_floor(curb_region, brush_shape)
   end

   return road
end

-- add `box` to the `floor` region.  the caller must ensure that the
-- box *only* overlaps with this segement of floor.  see :add_floor()
-- for more details.
--
--    @param floor - the floor to extend
--    @param floor_region - the area of the new floor segment
--    @param brush_shape - the brush for the new region
--
function BuildService:_merge_overlapping_floor_trivial(floor, floor_region, brush_shape)
   floor:add_component('stonehearth:floor')
            :add_region_to_floor(floor_region, brush_shape)
end

-- create a new floor of size `box` to `building`.  it is up to the caller
-- to ensure the new floor doesn't overlap with any other floor in the
-- building!
--
--    @param building - the building to contain the new floor
--    @param floor_uri - the uri to type of floor we'd like to add
--    @param box - the area of the new floor segment
--
function BuildService:_add_new_floor_to_building(building, floor_uri, floor_region, brush_shape)
   return self:_create_blueprint(building, floor_uri, Point3.zero, function(floor)
         self:_merge_overlapping_floor_trivial(floor, floor_region, brush_shape)
      end)
end

function BuildService:_add_new_road_to_building(building, floor_uri, floor_region, brush_shape)
   local bp, fab = self:_add_new_floor_to_building(building, floor_uri, floor_region, brush_shape)
   bp:get_component('stonehearth:floor'):set_category(constants.floor_category.ROAD)
   local fc = fab:get_component('stonehearth:fabricator')
   local move_mod = fc:get_project():add_component('movement_modifier_shape')
   move_mod:set_region(fc:get_project():get_component('region_collision_shape'):get_region())
   return bp
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
      self:_merge_building_into(merge_into, building)
   end
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


-- convert a 2d edge point to the proper 3d coordinate.  we want to put columns
-- 1-unit removed from where the floor is for each edge, so we add in the
-- accumualted normal for both the min and the max, with one small wrinkle:
-- the edges returned by :each_edge() live in the coordinate space of the
-- grid tile *lines* not the grid tile.  a consequence of this is that points
-- whose normals point in the positive direction end up getting pushed out
-- one unit too far.  try drawing a 2x2 cube and looking at each edge point +
-- accumulated normal in grid-tile space (as opposed to grid-line space) to
-- prove this to yourself if you don't believe me.
function BuildService:_edge_point_to_point(edge_point)
   local point = Point3(edge_point.location.x, 0, edge_point.location.y)
   if edge_point.accumulated_normals.x <= 0 then
      point.x = point.x + edge_point.accumulated_normals.x
   end
   if edge_point.accumulated_normals.y <= 0 then
      point.z = point.z + edge_point.accumulated_normals.y
   end
   return point
end


function BuildService:add_wall(session, columns_uri, walls_uri, p0, p1, normal)
   -- look for floor that we can merge into.
   local c0 = self:_get_blueprint_at_point(p0)
   local c1 = self:_get_blueprint_at_point(p1)
   local either_column = c0 or c1
   local building
   if c0 and c1 then
      local b0 = build_util.get_building_for(c0)
      local b1 = build_util.get_building_for(c1)
      assert(b0 == b1) -- merge required
      building = b0
   elseif either_column then
      building = build_util.get_building_for(either_column)
   else
      building = self:_create_new_building(session, p0)
   end

   assert(building)
   local wall = self:_add_wall_span(building, p0, p1, normal, columns_uri, walls_uri)
   return wall
end

-- add walls around all the floor segments for the specified `building` which
-- do not already have walls around them.
--
--    @param session - the session for the player initiating the request
--    @param response - a response object which we'll write the result into
--    @param building - the building whose floor we need to put walls around
--    @param columns_uri - the type of columns to generate
--    @param walls_uri - the type of walls to generate
--
function BuildService:grow_walls_command(session, response, building, columns_uri, walls_uri)
   local success = self:do_command('grow_walls', response, function()
         self:grow_walls(building, columns_uri, walls_uri)
      end)
   return success or nil
end

function BuildService:grow_walls(building, columns_uri, walls_uri)
   -- until we are smarter about the way we build walls, refuse to grow anything
   -- if any wall exists.  this prevents many stacking wall issues.
   local structures = building:get_component('stonehearth:building')
                              :get_all_structures()
   if next(structures['stonehearth:wall']) then
      self._log:info('already have walls in building %s.  not growing.', building)
      return
   end

   -- accumulate all the floor tiles in the building into a single, opaque region
   local floor_region = building:get_component('stonehearth:building')
                                    :calculate_floor_region()


   local origin = radiant.entities.get_world_grid_location(building)

   -- convert each 2d edge to 3d min and max coordinates and add a wall span
   -- for each one.
   local edges = floor_region:get_edge_list()
   for edge in edges:each_edge() do
      local min = self:_edge_point_to_point(edge.min) + origin
      local max = self:_edge_point_to_point(edge.max) + origin
      local normal = Point3(edge.normal.x, 0, edge.normal.y)

      if min ~= max then
         self:_add_wall_span(building, min, max, normal, columns_uri, walls_uri)
      end
   end
end


-- Pops a roof on a building with no roof. 
--
--    @param session - the session for the player initiating the request
--    @param response - a response object which we'll write the result into
--    @param building - the building to pop the roof onto
--    @param roof_uri - what kind of roof to make

function BuildService:grow_roof_command(session, response, building, roof_uri, options)
   local roof
   local success = self:do_command('grow_roof', response, function()
         roof = self:grow_roof(building, roof_uri, options)
      end)

   if success then
      if roof then     
         response:resolve({
            new_selection = roof:get_component('stonehearth:construction_progress'):get_fabricator_entity()
         })
      else
         response:reject({
            error = 'failed to grow roof'
         })
      end
   end
end

function BuildService:grow_roof(building, roof_uri, options)
   local structures = building:get_component('stonehearth:building')
                              :get_all_structures()
   if next(structures['stonehearth:roof']) then
      self._log:info('already have roof in building %s.  not growing.', building)
      return
   end

   -- compute the xz cross-section of the roof by growing the floor
   -- region by 2 voxels in every direction
   local region2 = building:get_component('stonehearth:building')
                              :calculate_floor_region()
                              :inflated(Point2(2, 2))

   -- now make the roof!
   local height = constants.STOREY_HEIGHT
   local roof_location = Point3(0, height - 1, 0)
   local roof = self:_create_blueprint(building, roof_uri, roof_location, function(roof_entity)
         roof_entity:add_component('stonehearth:construction_data')
                        :apply_nine_grid_options(options)
         roof_entity:add_component('stonehearth:roof')
                        :cover_region2(region2)
      end)

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
--    @param column_uri - the type of column to create if there's nothing at `point`
--
function BuildService:_fetch_column_at_point(building, point, column_uri)
   return self:_fetch_blueprint_at_point(building, point, column_uri, function(column)
         column:add_component('stonehearth:column')
                  :layout()
      end)
end

-- adds a span of walls between `min` and `max`, creating columns and
-- wall segments as required `min` must be less than `max` and the
-- requests span must be grid aligned (no diagonals!)
--    @param building - the building to put all the new objects in
--    @param min - the start of the wall span. 
--    @param max - the end of the wall span.
--    @param columns_uri - the type of columns to generate
--    @param walls_uri - the type of walls to generate
--
function BuildService:_add_wall_span(building, min, max, normal, columns_uri, wall_uri)
   local col_a = self:_fetch_column_at_point(building, min, columns_uri)
   local col_b = self:_fetch_column_at_point(building, max, columns_uri)
   if col_a and col_b then
      return self:_create_wall(building, col_a, col_b, normal, wall_uri)
   end
end

-- creates a wall inside `building` connecting `column_a` and `column_b`
-- pointing in the direction of `normal`.  
--    @param building - the building to add the wall to
--    @param column_a - one of the columns to attach the wall to
--    @param column_b - the other column to attach the wall to
--    @param normal - a Point3 pointing "outside" of the wall
--    @param wall_uri - the type of wall to build
--
function BuildService:_create_wall(building, column_a, column_b, normal, wall_uri)
   return self:_create_blueprint(building, wall_uri, Point3.zero, function(wall)      
         wall:add_component('stonehearth:wall')
                  :connect_to(column_a, column_b, normal)
                  :layout()

         wall:add_component('stonehearth:construction_data')
                  :set_normal(normal)

         wall:add_component('stonehearth:construction_progress')
                     :add_dependency(column_a)
                     :add_dependency(column_b)
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
   end
end

function BuildService:add_fixture_fabricator(fixture_blueprint, fixture_or_uri, normal, rotation)
   -- `fixture` is actually a blueprint of the fixture, not the actual fixture
   -- itself.  change the material we use to render it and hook up a fixture_fabricator
   -- to help build it.
   fixture_blueprint:add_component('render_info')
                        :set_material('materials/blueprint.material.xml')
   
   local fab_component = fixture_blueprint:add_component('stonehearth:fixture_fabricator')
   fab_component:start_project(fixture_or_uri, normal, rotation)

   self:_bind_fabricator_to_blueprint(fixture_blueprint, fixture_blueprint, 'stonehearth:fixture_fabricator')

   return fab_component
end

function BuildService:add_fixture(parent_entity, fixture_or_uri, location, normal, rotation)
   if not build_util.is_blueprint(parent_entity) then
      self._log:info('cannot place fixture %s on non-blueprint entity %s', fixture_uri, parent_entity)
      return
   end

   if not normal then
      normal = parent_entity:get_component('stonehearth:construction_data')
                              :get_normal()
   end
   assert(normal)
   
   local _, fixture_blueprint

   local _, _, fixture_ghost_uri = entity_forms.get_uris(fixture_or_uri)
   fixture_blueprint = radiant.entities.create_entity(fixture_ghost_uri)
   fixture_blueprint:set_debug_text('fixture blueprint')
   
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


   self:add_fixture_fabricator(fixture_blueprint, fixture_or_uri, normal, rotation)

   fixture_blueprint:add_component('stonehearth:construction_progress')
                        :add_dependency(parent_entity)

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


-- replace the `old` object with a brand new one created from `new_uri`, keeping
-- the structure of the building in-tact
--
--    @param session - the session for the player initiating the request
--    @param response - a response object which we'll write the result into
--    @param old - the old blueprint to destroy
--    @param new_uri - the uri of the new guy to take `old`'s place
--
function BuildService:substitute_blueprint_command(session, response, old, new_uri)
   local replaced
   local success = self:do_command('substitute_blueprint', response, function()
         replaced = self:_substitute_blueprint(old, new_uri)
      end)

   if success then   
      if not replaced then
         return false
      end
      
      -- set the fabricator as the new selected entity to preserve the illusion that
      -- the entity selected in the editor got changed.
      local replaced_fab = replaced:get_component('stonehearth:construction_progress'):get_fabricator_entity()
      response:resolve({
         new_selection = replaced_fab
      })
   end
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
   local cd = blueprint:get_component('stonehearth:construction_data')
   if cd then
      -- apply the new options to the blueprint      
      cd:apply_nine_grid_options(options)

      -- copy the options to the project.
      local fab = cd:get_fabricator_entity()
      local proj = fab:get_component('stonehearth:fabricator')
                           :get_project()
      proj:get_component('stonehearth:construction_data')
               :clone_from(blueprint)
      return true
   end
   return false
end

-- called by the roof to add a patch wall to the specified building. 
--
--    @param building - the building which needs a new patch wall.
--    @param wall_uri - the type of wall to create
--    @param normal - the normal for the wall
--    @param position - the local coordinate of the wall to add
--    @param region - the exact shape of the patch wall
--
function BuildService:add_patch_wall_to_building(building, wall_uri, normal, position, region)
   self:_create_blueprint(building, wall_uri, position, function(wall)
         wall:add_component('stonehearth:wall')
                  :create_patch_wall(normal, region)
                  :layout()

         -- we're going to cheat a little bit here.  it's very very difficult to
         -- get scaffolding building looking 'proper' on patch walls.  to do a good
         -- job, we need to make sure the scaffolding only gets built where the roof
         -- exists to support it and avoid all sorts of issues with "reachable but
         -- unsupported" pieces of wall being created (since walls can be built diagonally)
         -- it turns out to look much better to pound up patch walls like columns, so
         -- do that until we have more time to invest in building patch walls like every
         -- other wall.
         wall:add_component('stonehearth:construction_data')
                  :set_project_adjacent_to_base(true)
                  :set_needs_scaffolding(false)
      end)
end

function BuildService:create_ladder_command(session, response, ladder_uri, location, normal)
   normal = ToPoint3(normal)
   location = ToPoint3(location)
   self._sv.scaffolding_manager:request_ladder_to(location, normal, true)
   return true
end

function BuildService:remove_ladder_command(session, response, ladder_entity)
   if radiant.util.is_a(ladder_entity, Entity) then
      local ladder = ladder_entity:get_component('stonehearth:ladder')
      if ladder then
         local base = radiant.entities.get_world_grid_location(ladder_entity)
         self._sv.scaffolding_manager:remove_ladder(base)
      end
   end
   return true
end

function BuildService:request_ladder_to(climb_to, normal)
   return self._sv.scaffolding_manager:request_ladder_to(climb_to, normal)
end

function BuildService:instabuild_command(session, response, building)
   -- get everything built.
   self:instabuild(building)
   return true
end

function BuildService:instabuild(building)
   self:_call_all_children(building, function(entity)
         local cp = entity:get_component('stonehearth:construction_progress')
         if cp and entity:get_uri() ~= 'stonehearth:scaffolding' then
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

return BuildService

