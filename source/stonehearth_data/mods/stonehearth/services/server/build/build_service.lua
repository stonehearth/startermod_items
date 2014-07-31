local constants = require('constants').construction
local BuildUndoManager = require 'services.server.build.build_undo_manager'
local Rect2 = _radiant.csg.Rect2
local Cube3 = _radiant.csg.Cube3
local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3
local Region2 = _radiant.csg.Region2
local Region3 = _radiant.csg.Region3

local STRUCTURE_COMPONENTS = {
   'stonehearth:wall',
   'stonehearth:floor',
   'stonehearth:column',
   'stonehearth:roof',
}

-- these are quite annoying.  we can get rid of them by implementing and using
-- LuaToProto <-> ProtoToLua in the RPC layer (see lib/typeinfo/dispatcher.h)
local function ToPoint3(pt)
   return Point3(pt.x, pt.y, pt.z)
end
local function ToCube3(box)
   return Cube3(ToPoint3(box.min), ToPoint3(box.max))
end


local BuildService = class()

function BuildService:__init(datastore)
end

function BuildService:initialize()
   self._undo = BuildUndoManager()

   self._sv = self.__saved_variables:get_data()
   self.__saved_variables:set_controller(self)

   if not self._sv.next_building_id then
      self._sv.next_building_id = 1 -- used to number newly created buildings
      self.__saved_variables:mark_changed()
   end
end

function BuildService:set_active(building, enabled)
   local function _set_active_recursive(blueprint, enabled)
      local ec = blueprint:get_component('entity_container')  
      if ec then
         for id, child in ec:each_child() do
            _set_active_recursive(child, enabled)
         end
      end
      local c = blueprint:get_component('stonehearth:construction_progress')
      if c then
         c:set_active(enabled)
      end
   end  
   _set_active_recursive(building, enabled)
end

function BuildService:set_teardown(blueprint, enabled)
   local function _set_teardown_recursive(blueprint)
      local ec = blueprint:get_component('entity_container')  
      if ec then
         for id, child in ec:each_child() do         
            if child and child:is_valid() then
               _set_teardown_recursive(child, enabled)
            end
         end
      end
      local cp = blueprint:get_component('stonehearth:construction_progress')
      if cp then
         cp:set_teardown(enabled)
      end
   end  
   _set_teardown_recursive(blueprint, enabled)
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
   self._undo:begin_transaction('add_floor')
   local floor = self:_add_floor(session, floor_uri, ToCube3(box), brush_shape)
   self._undo:end_transaction('add_floor')

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

function BuildService:add_wall_command(session, response, columns_uri, walls_uri, p0, p1, normal)
   self._undo:begin_transaction('add_wall')
   local wall = self:_add_wall(session, columns_uri, walls_uri, ToPoint3(p0), ToPoint3(p1), ToPoint3(normal))
   self._undo:end_transaction('add_wall')

   if wall then
      local wall_fab = wall:get_component('stonehearth:construction_progress'):get_fabricator_entity()      
      response:resolve({
         new_selection = wall_fab
      })
   else
      response:reject({ error = 'could not create wall' })      
   end
end

function BuildService:_add_floor(session, floor_uri, box, brush_shape)
   local floor
   local floor_region = Region3(box)
   local overlap = Cube3(Point3(box.min.x - 1, box.min.y, box.min.z - 1),
                         Point3(box.max.x + 1, box.max.y, box.max.z + 1))

   -- look for floor that we can merge into.
   local all_overlapping_floor = radiant.terrain.get_entities_in_cube(overlap, function(entity)
         if not self:is_blueprint(entity) then
            return false
         end
         if self:_get_structure_type(entity) == 'floor' then
            return true
         end

         local dst = entity:get_component('destination')
         if dst then
            floor_region:subtract_region(dst:get_region():get())
         end
         return false
      end)

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

function BuildService:_erase_floor(session, box)
   local floor

   -- look for floor that we can merge into.
   local all_overlapping_floor = radiant.terrain.get_entities_in_cube(box, function(entity)
         return self:is_blueprint(entity) and self:_get_structure_type(entity) == 'floor'
      end)

   for _, floor in pairs(all_overlapping_floor) do
      floor:add_component('stonehearth:floor')
               :remove_region_from_floor(Region3(box))
   end
end

function BuildService:erase_floor_command(session, response, box)
   self._undo:begin_transaction('add_wall')
   self:_erase_floor(session, ToCube3(box))
   self._undo:end_transaction('add_wall')
   return true
end

-- adds a new fabricator to blueprint.  this creates a new 'stonehearth:entities:fabricator'
-- entity, adds a fabricator component to it, and wires that fabricator up to the blueprint.
-- See `_init_fabricator` for more details.
--    @param blueprint - The blueprint which needs a new fabricator.
--
function BuildService:_add_fabricator(blueprint)
   local fabricator = radiant.entities.create_entity('stonehearth:entities:fabricator')

   local blueprint_mob = blueprint:add_component('mob')
   local parent = blueprint_mob:get_parent()
   if parent then
      parent:add_component('entity_container'):add_child(fabricator)
   end
   local transform = blueprint_mob:get_transform()
   fabricator:add_component('mob'):set_transform(transform)
   fabricator:set_debug_text('(Fabricator for ' .. tostring(blueprint) .. ')')
   
   if blueprint:get_component('stonehearth:construction_data') then
      fabricator:add_component('stonehearth:fabricator')
                     :start_project(blueprint)
      blueprint:add_component('stonehearth:construction_progress')   
                     :set_fabricator_entity(fabricator)
   end   
   return fabricator
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

   -- make the owner of the blueprint the same as the owner as of the building
   blueprint:add_component('unit_info')
            :set_player_id(radiant.entities.get_player_id(building))
            :set_faction(radiant.entities.get_faction(building))

   blueprint:add_component('destination')
               :set_region(_radiant.sim.alloc_region())

   -- add the blueprint to the building's entity container and wire up the
   -- building entity pointer in the construction_progress component.
   blueprint:add_component('stonehearth:construction_progress')
               :set_building_entity(building)

   -- make sure the building depends on the child we're adding to it.  this
   -- will guarantee the building won't be flagged as finished until all the
   -- children are finished
   building:add_component('stonehearth:construction_progress')
               :add_dependency(blueprint)

   radiant.entities.add_child(building, blueprint, offset)

   -- initialize...
   init_fn(blueprint)

   -- if the blueprint does not yet have a fabricator, go ahead and create one
   -- now.
   local fabricator = self:_add_fabricator(blueprint)

   -- track the structure in the building component.  the building component is
   -- responsible for cross-structure interactions (e.g. sharing scaffolding)
   building:get_component('stonehearth:building')
               :add_structure(blueprint)

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
           :set_faction(session.faction)
           

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
function BuildService:_merge_overlapping_floor(existing_floor, floor_uri, floor_region, brush_shape)
   local id, floor = next(existing_floor)
   local id, next_floor = next(existing_floor, id)
   if not next_floor then
      -- exactly 1 overlapping floor.  just modify the region of that floor.
      -- pretty easy
      self:_merge_overlapping_floor_trivial(floor, floor_uri, floor_region, brush_shape)
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

   -- now do the merge...
   local buildings_to_merge = {}
   local floor_building = self:get_building_for(floor)
   for _, old_floor in pairs(existing_floor) do
      local building = self:get_building_for(old_floor)
      if building ~= floor_building then
         buildings_to_merge[building:get_id()] = building
      end
   end
   self:_merge_buildings_into(floor_building, buildings_to_merge)

   -- now we know all the floors are of the same building.  merge all the floors
   -- into the single floor we picked out earlier. 
   for _, old_floor in pairs(existing_floor) do
      if old_floor ~= floor then
         floor:get_component('stonehearth:floor'):merge_with(old_floor)
      end
   end

   -- sweet!  now we can do the trivial merge
    self:_merge_overlapping_floor_trivial(floor, floor_uri, floor_region, brush_shape)
    return floor
end

-- add `box` to the `floor` region.  the caller must ensure that the
-- box *only* overlaps with this segement of floor.  see :add_floor()
-- for more details.
--
--    @param floor - the floor to extend
--    @param floor_uri - the uri to type of floor we'd like to add
--    @param box - the area of the new floor segment
--
function BuildService:_merge_overlapping_floor_trivial(floor, floor_uri, floor_region, brush_shape)
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
         self:_merge_overlapping_floor_trivial(floor, floor_uri, floor_region, brush_shape)
      end)
end

-- return the building the `blueprint` is contained in
--
--    @param blueprint - the blueprint whose building you're interested in
--
function BuildService:get_building_for(blueprint)
   local cp = blueprint:get_component('stonehearth:construction_progress')
   return cp and cp:get_building_entity()
end

-- return whether or not the specified `entity` is a blueprint.  only blueprints
-- have stonehearth:construction_progress components.
--
--    @param entity - the entity to be tested for blueprintedness
--
function BuildService:is_blueprint(entity)
   return entity:get_component('stonehearth:construction_progress') ~= nil
end

-- return the building the specified buildprint is attached to, assuming it
-- is a blueprint to begin with!
--
--     @param entity - the blueprint whose building you're looking for
--
function BuildService:get_building_for_blueprint(entity)
   local cp = entity:get_component('stonehearth:construction_progress')
   if cp then
      return cp:get_building_entity()
   end
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
   self:unlink_entity(building)
end

function BuildService:_add_wall(session, columns_uri, walls_uri, p0, p1, normal)
   -- look for floor that we can merge into.
   local c0 = self:_get_blueprint_at_point(p0)
   local c1 = self:_get_blueprint_at_point(p1)
   local either_column = c0 or c1
   local building
   if c0 and c1 then
      local b0 = self:get_building_for(c0)
      local b1 = self:get_building_for(c1)
      assert(b0 == b1) -- merge required
      building = b0
   elseif either_column then
      building = self:get_building_for(either_column)
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
   self._undo:begin_transaction('grow_walls')
   self:_grow_walls(building, columns_uri, walls_uri)
   self._undo:end_transaction('grow_walls')
   response:resolve(true)
end

function BuildService:_grow_walls(building, columns_uri, walls_uri)
   -- accumulate all the floor tiles in the building into a single, opaque region
   local floor_region = building:add_component('stonehearth:building')
                                    :calculate_floor_region()

   -- convert a 2d edge point to the proper 3d coordinate.  we want to put columns
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

   local origin = radiant.entities.get_world_grid_location(building)

   -- convert each 2d edge to 3d min and max coordinates and add a wall span
   -- for each one.
   local edges = floor_region:get_edge_list()
   for edge in edges:each_edge() do
      local min = edge_point_to_point(edge.min) + origin
      local max = edge_point_to_point(edge.max) + origin
      local normal = Point3(edge.normal.x, 0, edge.normal.y)

      self:_add_wall_span(building, min, max, normal, columns_uri, walls_uri)   
   end
end


-- Pops a roof on a building with no roof. 
--
--    @param session - the session for the player initiating the request
--    @param response - a response object which we'll write the result into
--    @param building - the building to pop the roof onto
--    @param roof_uri - what kind of roof to make

function BuildService:grow_roof_command(session, response, building, roof_uri, options)
   self._undo:begin_transaction('grow_roof')
   local roof = self:_grow_roof(building, roof_uri, options)
   self._undo:end_transaction('grow_roof')

   response:resolve({
      new_selection = roof
   })
end

function BuildService:_grow_roof(building, roof_uri, options)
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
                        :apply_options(options)
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
         return self:is_blueprint(entity)
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
      assert(self:get_building_for(blueprint) == building)
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

         wall:add_component('stonehearth:construction_progress')
                     :add_dependency(column_a)
                     :add_dependency(column_b)
      end)
end


-- creates a portal inside `wall_entity` at `location`  
--    @param wall_entity - the wall you'd like to contain the portal
--    @param portal_uri - the type of portal to create
--    @param location - where to put the portal, in wall-local coordinates
--
function BuildService:add_portal_command(session, response, wall_entity, portal_uri, location)
   self._undo:begin_transaction('add_portal')
   local portal = self:_add_portal(wall_entity, portal_uri, location)
   self._undo:end_transaction('add_portal')

   response:resolve({
      new_selection = portal
   })   
end

function BuildService:_add_portal(wall_entity, portal_uri, location)
   local wall = wall_entity:get_component('stonehearth:wall')
   if wall then
      local portal_blueprint_uri = portal_uri
      local data = radiant.entities.get_component_data(portal_uri, 'stonehearth:entity_forms')
      if data then
         portal_blueprint_uri = data.ghost_form
      end

      local building = self:get_building_for(wall_entity)
      local portal_blueprint = radiant.entities.create_entity(portal_blueprint_uri)

      portal_blueprint:add_component('unit_info')
                        :set_player_id(radiant.entities.get_player_id(building))
                        :set_faction(radiant.entities.get_faction(building))

      -- add the new portal to the wall and reconstruct the shape.
      wall:add_portal(portal_blueprint, location)
          :layout()

      -- `portal` is actually a blueprint of the fixture, not the actual fixture
      -- itself.  change the material we use to render it and hook up a fixture_fabricator
      -- to help build it.
      portal_blueprint:add_component('render_info')
                        :set_material('materials/blueprint.material.xml')
                        
      portal_blueprint:add_component('stonehearth:construction_progress')
                        :add_dependency(wall_entity)
                        :set_fabricator_entity(portal_blueprint, 'stonehearth:fixture_fabricator')

      portal_blueprint:add_component('stonehearth:fixture_fabricator')
                        :start_project(portal_uri)

      -- sadly, ordering matters here.  we cannot set the building until both
      -- the fabricator and blueprint have been fully initialized.
      portal_blueprint:add_component('stonehearth:construction_progress')
                        :set_building_entity(building)

      return portal_blueprint
   end
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
   self._undo:begin_transaction('subsitute_blueprint')
   local replaced = self:_substitute_blueprint(old, new_uri)
   self._undo:end_transaction('subsitute_blueprint')
   
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

function BuildService:_substitute_blueprint(old, new_uri)
   -- make sure the old building has a parent.  if it doesn't, it means the structure had
   -- been unlinked, and the client somehow sent us some state data (perhaps as a race)
   if not old:add_component('mob'):get_parent() then
      return
   end
   
   local building = self:get_building_for(old)   
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
         new:add_component('stonehearth:construction_progress')
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
   local building = self:get_building_for(entity)
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
   local cd = blueprint:get_component('stonehearth:construction_data')
   if cd then
      -- apply the new options to the blueprint      
      cd:apply_options(options)

      -- copy the options to the project.  this probably shouldn't be necessary. 
      -- project's don't need a construction data component, do they? (the only
      -- reason I can think of is to make it easy to find their building, but can't
      -- we do that with a helper method that assume's its' the mob's parent?)
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
      end)
end

return BuildService


