local constants = require('constants').construction
local voxel_brush_util = require 'services.server.build.voxel_brush_util'
local Rect2 = _radiant.csg.Rect2
local Cube3 = _radiant.csg.Cube3
local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3
local Region2 = _radiant.csg.Region2
local Region3 = _radiant.csg.Region3

local BuildService = class()

function BuildService:__init(datastore)
end

function BuildService:initialize()
   self._sv = self.__saved_variables:get_data()
   if not self._sv.next_building_id then
      self._sv.next_building_id = 1 -- used to number newly created buildings
      self.__saved_variables:mark_changed()
   end
end

function BuildService:restore(saved_variables)
end


--- Convert the proxy to an honest to goodness blueprint entity
-- The blueprint_map contains a map from proxy entity ids to the entities that
-- get created for those proxies.
function BuildService:_unpackage_proxy_data(proxy, blueprint_map)

   -- Create the blueprint for this proxy and update the blueprint_map
   local blueprint = radiant.entities.create_entity(proxy.entity_uri)
   blueprint_map[proxy.entity_id] = blueprint
   
   -- Initialize all the simple components.
   radiant.entities.set_faction(blueprint, self._faction)
   radiant.entities.set_player_id(blueprint, self._player_id)
   if proxy.components then
      for name, data in pairs(proxy.components) do
         blueprint:add_component(name, data)
      end
   end

   -- all blueprints have a 'stonehearth:construction_progress' component to track whether
   -- or not they're finished.  dependencies are also tracked here.
   local progress = blueprint:add_component('stonehearth:construction_progress')
  
   -- Initialize the construction_data and entity_container components.  We can't
   -- handle these with :load_from_json(), since the actual value of the entities depends on
   -- what gets created server-side.  So instead, look up the actual entity that
   -- got created in the entity map and shove that into the component.
   if proxy.dependencies then      
      for _, dependency_id in ipairs(proxy.dependencies) do
         local dep = blueprint_map[dependency_id];
         assert(dep, string.format('could not find dependency blueprint %d in blueprint map', dependency_id))
         progress:add_dependency(dep);
      end
   end
   if proxy.children then
      local ec = blueprint:add_component('entity_container')
      for _, child_id in ipairs(proxy.children) do
         local child_entity = blueprint_map[child_id]
         assert(child_entity, string.format('could not find child entity %d in blueprint map', child_id))
         ec:add_child(child_entity)
      end
   end

   return blueprint
end

function BuildService:build_structures(session, proxies)
   local root = radiant.entities.get_root_entity()
   local town = stonehearth.town:get_town(session.player_id)
   local result = {
      blueprints = {}
   }

   self._faction = session.faction
   self._player_id = session.player_id

   -- Create a new entity for each entry in the proxies list.  The client is
   -- responsible for creating the list in such a way that we can do this
   -- naievely (e.g. when we encounter an item in some proxy's children list, that
   -- child is guarenteed to have already been processed)
   local entity_map = {}
   for _, proxy in ipairs(proxies) do
      local blueprint = self:_unpackage_proxy_data(proxy, entity_map)
      
      -- If this proxy needs to be added to the terrain, go ahead and do that now.
      -- This will also create fabricators for all the descendants of this entity
      if proxy.add_to_build_plan then
         blueprint:add_component('unit_info')
                  :set_display_name(string.format('!! Building %d', radiant.get_realtime()))
         self:_begin_construction(blueprint)
         town:add_construction_blueprint(blueprint)

         result.blueprints[blueprint:get_id()] = blueprint
      end
   end

   for _, proxy in ipairs(proxies) do
      local blueprint = entity_map[proxy.entity_id]

      if proxy.building_id then
         local progress = blueprint:get_component('stonehearth:construction_progress')
         local building = entity_map[proxy.building_id]
         assert(building, string.format('could not find building %d in blueprint map', proxy.building_id))
         progress:set_building_entity(building);
      end
   
      -- xxx: this whole "borrow scaffolding from" thing should go away when we factor scaffolding
      -- out of the fabricator and into the build service!
      if proxy.loan_scaffolding_to then
         local cd = blueprint:add_component('stonehearth:construction_data')
         for _, borrower_id in ipairs(proxy.loan_scaffolding_to) do
            local borrower = entity_map[borrower_id]
            assert(borrower, string.format('could not find scaffolding borrower entity %d in blueprint map', borrower_id))
            cd:loan_scaffolding_to(borrower)
         end
      end
   end

   return result
end

function BuildService:set_active(building, enabled)
   local function _set_active_recursive(blueprint, enabled)
      local ec = blueprint:get_component('entity_container')  
      if ec then
         for id, child in ec:each_child() do
            _set_active_recursive(child, enabled)
         end
      end
      local cp = blueprint:get_component('stonehearth:construction_progress')
      if cp then
         cp:set_active(enabled)
      end
   end  
   _set_active_recursive(building, enabled)
end

function BuildService:set_teardown(blueprint, enabled)
   local function _set_teardown_recursive(blueprint)
      local ec = blueprint:get_component('entity_container')  
      if ec then
         for id, child in ec:each_child() do
            _set_teardown_recursive(child, enabled)
         end
      end
      local cp = blueprint:get_component('stonehearth:construction_progress')
      if cp then
         cp:set_teardown(enabled)
      end
   end  
   _set_teardown_recursive(blueprint, enabled)
end

function BuildService:_begin_construction(blueprint, location)
   -- create a new fabricator...   to... you know... fabricate
   local fabricator = self:create_fabricator_entity(blueprint)
   radiant.terrain.place_entity(fabricator, location)
end

function BuildService:create_fabricator_entity(blueprint)
   -- either you're a fabricator or you contain things which may be fabricators.  not
   -- both!
   local fabricator = radiant.entities.create_entity('stonehearth:entities:fabricator')
   self:_init_fabricator(fabricator, blueprint)
   self:_init_fabricator_children(fabricator, blueprint)
   return fabricator
end

function BuildService:_init_fabricator(fabricator, blueprint)
   local blueprint_mob = blueprint:add_component('mob')
   local parent = blueprint_mob:get_parent()
   if parent then
      parent:add_component('entity_container'):add_child(fabricator)
   end
   local transform = blueprint_mob:get_transform()
   fabricator:add_component('mob'):set_transform(transform)
   fabricator:set_debug_text('(Fabricator for ' .. tostring(blueprint) .. ')')
   
   if blueprint:get_component('stonehearth:construction_data') then
      local name = tostring(blueprint)
      fabricator:add_component('stonehearth:fabricator')
                     :start_project(name, blueprint)
      blueprint:add_component('stonehearth:construction_progress')   
                  :set_fabricator_entity(fabricator)
   end

end

function BuildService:_init_fabricator_children(fabricator, blueprint)
   local ec = blueprint:get_component('entity_container')  
   if ec then
      for id, child in ec:each_child() do
         local fc = self:create_fabricator_entity(child)
         fabricator:add_component('entity_container'):add_child(fc)
      end
   end
end

-- THERE BE DRAGONS HERE THERE BE DRAGONS HERE THERE BE DRAGONS HERE THERE BE DRAGONS HERE 
--
-- Stuff below the line is sane and will work with multiplayer.
--
-- Stuff above this line is suspect.  The proxy upload thing is really hard to get right
-- when many people are uploading proxies, and frankly I believe it will be much more
-- error prone going forward (e.g. editing, merging, etc.)
--
-- THERE BE DRAGONS HERE THERE BE DRAGONS HERE THERE BE DRAGONS HERE THERE BE DRAGONS HERE




-- Add a new floor segment to the world.  This will try to merge with existing buildings
-- if the floor overlaps some pre-existing floor or will create a new building to hold
-- the floor
--
--    @param session - the session for the player initiating the request
--    @param response - a response object which we'll write the result into
--    @param floor_uri - the uri to type of floor we'd like to add
--    @param box - the area of the new floor segment

function BuildService:add_floor(session, response, floor_uri, box)
   local floor

   -- look for floor that we can merge into.
   local existing_floor = radiant.terrain.get_entities_in_cube(box, function(entity)
         return self:_is_blueprint(entity) and self:_get_structure_type(entity) == 'floor'
      end)

   if not next(existing_floor) then
      -- there was no existing floor at all. create a new building and add a floor
      -- segment to it. 
      local building = self:_create_new_building(session, box.min)
      floor = self:_add_new_floor_to_building(building, floor_uri, box)
   else
      -- we overlapped some pre-existing floor.  merge this box into that floor,
      -- potentially merging multiple buildings together!
      floor = self:_merge_overlapping_floor(existing_floor, floor_uri, box)
   end
   
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

-- adds a new fabricator to blueprint.  this creates a new 'stonehearth:entities:fabricator'
-- entity, adds a fabricator component to it, and wires that fabricator up to the blueprint.
-- See `_init_fabricator` for more details.
--    @param blueprint - The blueprint which needs a new fabricator.
--
function BuildService:_add_fabricator(blueprint)
   local fabricator = radiant.entities.create_entity('stonehearth:entities:fabricator')
   self:_init_fabricator(fabricator, blueprint)
   return fabricator
end

-- adds a new `blueprint` entity to the specified `building` entity at the optional
-- location.  also handles making sure the blueprint is owned by the building owner
-- and creating a fabricator for the blueprint
--
--    @param building - The building entity which will contain the blueprint
--    @param blueprint - The blueprint to be added to the building
--    @param offset - (optional) a Point3 representing the offset in the building
--                      where the blueprint is located
--
function BuildService:_add_to_building(building, blueprint, offset)
   -- make the owner of the blueprint the same as the owner as of the building
   blueprint:add_component('unit_info')
            :set_player_id(radiant.entities.get_player_id(building))
            :set_faction(radiant.entities.get_faction(building))

   -- if the blueprint doesn't have a destination region, go ahead and add one
   local dst = blueprint:add_component('destination')
   if not dst:get_region() then
      dst:set_region(_radiant.sim.alloc_region())
   end

   -- add the blueprint to the building's entity container and wire up the
   -- building entity pointer in the construction_progress component.
   local cp = blueprint:add_component('stonehearth:construction_progress')
   cp:set_building_entity(building)

   radiant.entities.add_child(building, blueprint, offset)

   -- if the blueprint does not yet have a fabricator, go ahead and create one
   -- now.
   local fabricator = cp:get_fabricator_entity()
   if not fabricator then
      fabricator = self:_add_fabricator(blueprint)
   end
   return fabricator
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
   
   -- give the building a unique name and establish ownership.
   building:add_component('unit_info')
           :set_display_name(string.format('Building No.%d', self._sv.next_building_id))
           :set_player_id(session.player_id)
           :set_faction(session.faction)
           

   self._sv.next_building_id = self._sv.next_building_id + 1
   self.__saved_variables:mark_changed()

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
function BuildService:_merge_overlapping_floor(existing_floor, floor_uri, box)
   local id, floor = next(existing_floor)
   local id, next_floor = next(existing_floor, id)
   if not next_floor then
      -- exactly 1 overlapping floor.  just modify the region of that floor.
      -- pretty easy
      return self:_merge_overlapping_floor_trivial(floor, floor_uri, box)
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

   -- now do the merge...
   local buildings_to_merge = {}
   local floor_building = self:_get_building_for(floor)
   for _, old_floor in pairs(existing_floor) do
      local building = self:_get_building_for(old_floor)
      if building ~= floor_building then
         buildings_to_merge[building:get_id()] = building
      end
   end
   self:_merge_buildings_into(floor_building, buildings_to_merge)

   -- now we know all the floors are of the same building.  merge all the floors
   -- into the single floor we picked out earlier.
   local floor_rgn = floor:get_component('destination'):get_region()
   local floor_location = radiant.entities.get_location_aligned(floor)
   floor_rgn:modify(function(cursor)
         for _, old_floor in pairs(existing_floor) do
            if old_floor ~= floor then
               local floor_offset = radiant.entities.get_location_aligned(old_floor) - floor_location                                    
               local old_floor_region = old_floor:get_component('destination'):get_region()
               for cube in old_floor_region:get():each_cube() do
                  cursor:add_unique_cube(cube:translated(floor_offset))
               end
               radiant.entities.destroy_entity(old_floor)
            end
         end
      end)

   -- sweet!  now we can do the trivial merge
   return self:_merge_overlapping_floor_trivial(floor, floor_uri, box)
end

-- add `box` to the `floor` region.  the caller must ensure that the
-- box *only* overlaps with this segement of floor.  see :add_floor()
-- for more details.
--
--    @param floor - the floor to extend
--    @param floor_uri - the uri to type of floor we'd like to add
--    @param box - the area of the new floor segment
--
function BuildService:_merge_overlapping_floor_trivial(floor, floor_uri, box)
   local region = floor:get_component('destination'):get_region()
   local origin = radiant.entities.get_world_grid_location(floor)
   region:modify(function(cursor)
         cursor:add_cube(box:translated(-origin))
      end)
   return floor
end

-- create a new floor of size `box` to `building`.  it is up to the caller
-- to ensure the new floor doesn't overlap with any other floor in the
-- building!
--
--    @param building - the building to contain the new floor
--    @param floor_uri - the uri to type of floor we'd like to add
--    @param box - the area of the new floor segment
--
function BuildService:_add_new_floor_to_building(building, floor_uri, box)
   local floor = radiant.entities.create_entity(floor_uri)

   local origin = radiant.entities.get_world_grid_location(building)
   local region = _radiant.sim.alloc_region()
   region:modify(function(c)
         c:add_unique_cube(box:translated(-origin))
      end)
   floor:add_component('destination'):set_region(region)

   self:_add_to_building(building, floor)
   return floor
end

-- return the building the `blueprint` is contained in
--
--    @param blueprint - the blueprint whose building you're interested in
--
function BuildService:_get_building_for(blueprint)
   local cp = blueprint:get_component('stonehearth:construction_progress')
   return cp and cp:get_building_entity()
end

-- return whether or not the specified `entity` is a blueprint.  only blueprints
-- have stonehearth:construction_progress components.
--
--    @param entity - the entity to be tested for blueprintedness
--
function BuildService:_is_blueprint(entity)
   return entity:get_component('stonehearth:construction_progress') ~= nil
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
--    @param merge_into - a building entith which will contain all the
--                        children of all the buildings in `building`
--    @param building - the building which should be merged into `merge_into`,
--                      then destroyed.
--
function BuildService:_merge_building_into(merge_into, building)
   local building_offset = radiant.entities.get_world_grid_location(building) - 
                           radiant.entities.get_world_grid_location(merge_into)

   local container = merge_into:get_component('entity_container')
   for id, child in building:get_component('entity_container'):each_child() do
      local mob = child:get_component('mob')
      local child_offset = mob:get_grid_location()

      container:add_child(child)
      mob:set_location_grid_aligned(child_offset + building_offset)      
   end
   radiant.entities.destroy_entity(building)
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
function BuildService:grow_walls(session, response, building, columns_uri, walls_uri)
   -- accumulate all the floor tiles in the building into a single, opaque region
   local floor_region = Region2()
   local ec = building:get_component('entity_container')
   for _, entity in ec:each_child() do
      if self:_is_blueprint(entity) and self:_get_structure_type(entity) == 'floor' then
         local rgn = entity:get_component('destination'):get_region():get()
         for cube in rgn:each_cube() do
            local rect = Rect2(Point2(cube.min.x, cube.min.z),
                               Point2(cube.max.x, cube.max.z))
            floor_region:add_unique_cube(rect)
         end
      end
   end
   local origin = radiant.entities.get_world_grid_location(building)
   local edges = floor_region:get_edge_list()

   -- convert a 2d edge point to the proper 3d coorinate.  we want to put columns
   -- 1-unit removed from where the floor is for each edge, so we add in the
   -- accumualted normal for both the min and the max, with one small wrinkle:
   -- the edges returned by :each_edge() live in the coordinate space of the
   -- grid tile *lines* not the grid tile.  a consequence of this is that points
   -- whose normals point in the positive direction end up getting pushed out
   -- one unit too far.  try drawing a 2x2 cube and looking at each edge point +
   -- accumulated normal in grid-tile space (as opposed to grid-line space) to
   -- prove this to yourself if you don't believe me.
   local function edge_point_to_point(edge_point)
      local point = Point3(edge_point.location.x, 0, edge_point.location.y)
      if edge_point.accumulated_normals.x <= 0 then
         point.x = point.x + edge_point.accumulated_normals.x
      end
      if edge_point.accumulated_normals.y <= 0 then
         point.z = point.z + edge_point.accumulated_normals.y
      end
      return point
   end

   -- convert each 2d edge to 3d min and max coordinates and add a wall span
   -- for each one.
   for edge in edges:each_edge() do
      local min = edge_point_to_point(edge.min) + origin
      local max = edge_point_to_point(edge.max) + origin
      local normal = Point3(edge.normal.x, 0, edge.normal.y)

      self:_add_wall_span(building, min, max, normal, columns_uri, walls_uri)
   end
end

-- returns the blueprint at the specified world `point`
--    @param point - the world space coordinate of where you're looking
--
function BuildService:_get_blueprint_at_point(point)
   local entities = radiant.terrain.get_entities_at_point(point, function(entity)
         return self:_is_blueprint(entity)
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
      assert(self:_get_building_for(blueprint) == building)
   else
      local origin = radiant.entities.get_world_grid_location(building)
      blueprint = radiant.entities.create_entity(blueprint_uri)
      init_fn(blueprint)
      self:_add_to_building(building, blueprint, point - origin)
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
   return self:_fetch_blueprint_at_point(building, point, column_uri, function(blueprint)
         local rgn = _radiant.sim.alloc_region()
         rgn:modify(function(cursor)
               cursor:add_unique_cube(Cube3(Point3(0, 0, 0), Point3(1, constants.STOREY_HEIGHT, 1)))
            end)
         blueprint:add_component('destination'):set_region(rgn)
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
   self:_create_wall(building, col_a, col_b, normal, wall_uri)
end

-- creates a wall inside `building` connecting `column_a` and `column_b`
-- pointing in the direction of `normal`.  this automatically orients the
-- wall in the correct direction, meaning the actual wall position will either
-- be near `column_a` or `column_b`, depending (see implementation for details)
--    @param building - the building to add the wall to
--    @param column_a - one of the columns to attach the wall to
--    @param column_b - the other column to attach the wall to
--    @param normal - a Point3 pointing "outside" of the wall
--    @param wall_uri - the type of wall to build
--
function BuildService:_create_wall(building, column_a, column_b, normal, wall_uri)
   local pos_a = radiant.entities.get_location_aligned(column_a)
   local pos_b = radiant.entities.get_location_aligned(column_b)
   local origin = radiant.entities.get_world_grid_location(building)   

   -- figure out the tangent and normal coordinate indicies based on the direction
   -- of the normal
   local t, n
   if pos_a.x == pos_b.x then
      t, n = 'z', 'x'
   else
      t, n = 'x', 'z'
   end   
   assert(pos_a[n] == pos_b[n], 'points are not co-linear')
   assert(pos_a[t] <  pos_b[t], 'points are not sorted')

   -- the origin of the wall will be at the start or the end, depending on the
   -- normal.  to make the tiling of the voxels of the wall look good, ensure
   -- that the 0,0 voxel is never adjoining a shared column of *both* walls
   -- connected to that column.  otherwise, you get this weird mirror effect.
   local flip
   if normal.x ~= 0 then
      flip = normal.x < 0
   elseif normal.z ~= 0 then
      flip = normal.z > 0
   end
   if flip then
      pos_a, pos_b = pos_b, pos_a
   end

   -- compoute the width of the wall and a tangent vector which will march
   -- us from pos_a to pos_b.
   local tangent = Point3(0, 0, 0)
   local start_pt = Point3(0, 0, 0)
   local end_pt = Point3(1, constants.STOREY_HEIGHT, 1)

   if pos_a[t] < pos_b[t] then
      tangent[t] = 1
      end_pt[t] = pos_b[t] - pos_a[t] - 1
   else
      tangent[t] = -1
      start_pt[t] = -(pos_a[t] - pos_b[t] - 2)
   end

   -- if the tangent coordinate is 0, then there's no wall to create!
   if start_pt[t] == end_pt[t] then
      return
   end
   assert(start_pt.x < end_pt.x)
   assert(start_pt.y < end_pt.y)
   assert(start_pt.z < end_pt.z)

   --[[
   -- DO NOT DELETE THIS CODE!  I will use it soon.

   -- omfg... righthandedness is screwing with my brain.
   local rotations = {
      [-1] = { [-1] = 270, [1] = 180 },
      [ 1] = { [-1] = 0,   [1] = 90 },
   }
   self._rotation = rotations[tangent[t] ][normal[n] ]
   ]]
   
   local wall = radiant.entities.create_entity(wall_uri)
   local cd = wall:get_component('stonehearth:construction_data')
   cd:set_normal(normal)
   self:_add_to_building(building, wall, pos_a + tangent)

   -- paint once to get the depth of the wall
   local brush = voxel_brush_util.create_brush(cd:get_data())
   local model = brush:paint_once()
   local bounds = model:get_bounds()
   end_pt[n] = bounds.max[n]
   start_pt[n] = bounds.min[n]
   
   -- paint again to actually draw the wallprox
   bounds = Cube3(start_pt, end_pt)
   local collsion_shape = brush:paint_through_stencil(Region3(bounds))
   wall:get_component('destination'):get_region():modify(function(cursor)
      cursor:copy_region(collsion_shape)
   end)

   return wall
end

return BuildService


