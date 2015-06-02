local csg_lib = require 'lib.csg.csg_lib'

local constants = require('constants').construction
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local Point2 = _radiant.csg.Point2
local Rect2 = _radiant.csg.Rect2
local Region2 = _radiant.csg.Region2
local Quaternion = _radiant.csg.Quaternion
local NineGridBrush = _radiant.voxel.NineGridBrush

local build_util = {}

local roation_eps = 0.01
local INFINITE = 1000000

local SAVED_COMPONENTS = {
   ['stonehearth:wall'] = true,
   ['stonehearth:column'] = true,
   ['stonehearth:roof'] = true,
   ['stonehearth:floor'] = true,
   ['stonehearth:building'] = true,
   ['stonehearth:fixture_fabricator'] = true,      -- for placed item locations
   ['stonehearth:construction_data'] = true,       -- for nine grid info
   ['stonehearth:construction_progress'] = true,   -- for dependencies
}

function build_util.rotated_degrees(value, degrees)
   value = value + degrees
   if value >= 360 then
      value = value - 360
   end
   return value
end

function build_util.normal_to_rotation(normal)
   if normal == -Point3.unit_z then
      return 0
   elseif normal == -Point3.unit_x then
      return 90
   elseif normal == Point3.unit_z then
      return 180
   elseif normal == Point3.unit_x then
      return 270
   end
   assert(false, string.format('invalid normal %s in normal_to_rotation()', tostring(normal)))
end

function build_util.rotation_to_normal(rotation)
   if math.abs(rotation) < roation_eps then
      return Point3(0, 0, -1)
   elseif math.abs(rotation - 90) < roation_eps then
      return Point3(-1, 0, 0)
   elseif math.abs(rotation - 180) < roation_eps then
      return Point3(0, 0, 1)
   elseif math.abs(rotation - 270) < roation_eps then
      return Point3(1, 0, 0)
   end
   assert(false, string.format('non-aligned rotation %f in rotation_to_normal()', rotation))
end

local function for_each_child(entity, fn)
   local ec = entity:get_component('entity_container')
   if ec then
      for id, child in ec:each_child() do
         fn(id, child)
      end
   end
end

local function load_structure_from_template(entity, template, options, entity_map)
   assert(template and template.uri)

   if template.mob then
      local xf = template.mob
      entity:add_component('mob')
               :move_to(Point3(xf.position.x, xf.position.y, xf.position.z))
               :set_rotation(Quaternion(xf.orientation.w, xf.orientation.x, xf.orientation.y, xf.orientation.z))
   end
   
   if template.shape then
      local color_region = radiant.alloc_region3()
      color_region:modify(function(cursor)
            cursor:load(template.shape)
         end)

      local shape = radiant.alloc_region3()
      shape:modify(function(cursor)
            cursor:copy_region(color_region:get())
            cursor:set_tag(0)
         end)

      entity:add_component('stonehearth:construction_progress')
               :set_color_region(color_region)
      entity:add_component('stonehearth:construction_data')
               :set_color_region(color_region)

      if options.mode == 'preview' then
         entity:add_component('region_collision_shape')
                  :set_region(shape)
         entity:add_component('render_info')
                  :set_material('materials/place_template_preview.json')
      else
         entity:add_component('destination')
                  :set_region(shape)
      end
   end

   for name, data in pairs(template) do
      if SAVED_COMPONENTS[name] then
         -- if we're running on the client, there might not be a client component implemented.
         -- that's totally cool, just ignore those.
         local component = entity:add_component(name)
         if component and radiant.util.is_instance(component) then
            component:load_from_template(data, options, entity_map)
         end
      end
   end
end

local function load_all_structures_from_template(entity, template, options, entity_map)
   options = options or {}
   load_structure_from_template(entity, template, options, entity_map)
   if template.children then
      for orig_id, child_template in pairs(template.children) do
         local child = entity_map[tonumber(orig_id)]
         load_all_structures_from_template(child, child_template, options, entity_map)
         entity:add_component('entity_container')
                     :add_child(child)
      end
   end
end

local function create_template_entities(building, template)
   local entity_map = {}
   local player_id = radiant.entities.get_player_id(building)

   entity_map[tonumber(template.root.id)] = building
   
   local function create_entities(children)
      if children then
         for orig_id, o in pairs(children) do
            local entity = radiant.entities.create_entity(o.uri)
            entity:add_component('unit_info')
                     :set_player_id(player_id)

            entity_map[tonumber(orig_id)] = entity
            create_entities(o.children)
         end
      end
   end
   create_entities(template.root.children)
   return entity_map
end

local function save_entity_to_template(entity)
   local template = {}
   local uri = entity:get_uri()
   template.uri = uri

   if uri ~= 'stonehearth:build:prototypes:building' then
      template.mob = entity:get_component('mob')
                              :get_transform()
   end

   -- find the color bits and save them in `shape`
   local cp = entity:get_component('stonehearth:construction_progress')
   if cp then
      local color_region = cp:get_color_region()
      if color_region then
         template.shape = color_region:get()
      end
   end

   for name, _ in pairs(SAVED_COMPONENTS) do
      local component = entity:get_component(name)
      if component then
         template[name] = component:save_to_template()
      end
   end
   
   return template
end

local function save_all_structures_to_template(entity)
   -- compute the save order by walking the dependencies.  
   if not build_util.is_blueprint(entity) and
      not build_util.is_building(entity) then
      return nil
   end

   local template = save_entity_to_template(entity, SAVED_COMPONENTS)

   for_each_child(entity, function(id, child)
         local info = save_all_structures_to_template(child)
         if info then
            if not template.children then
               template.children = {}
            end
            template.children[child:get_id()] = info
         end
      end)

   return template
end


-- return whether or not the specified `entity` is a blueprint.  only blueprints
-- have stonehearth:construction_progress components (except buildings!)
--
--    @param entity - the entity to be tested for blueprintedness
--
function build_util.is_blueprint(entity, opt_component)
   if entity:get_component('stonehearth:construction_progress') == nil then
      return false
   end
   if build_util.is_building(entity) then
      return false
   end
   if opt_component ~= nil and entity:get_component(opt_component) == nil then
      return false
   end
   return true
end

function build_util.is_building(entity)
   return entity:get_component('stonehearth:building') ~= nil
end

function build_util.is_fabricator(entity)
   return entity:get_component('stonehearth:fabricator') ~= nil or
          entity:get_component('stonehearth:fixture_fabricator') ~= nil
end

function build_util.has_walls(building)
   for _, child in building:get_component('entity_container'):each_child() do
     if child:get_component('stonehearth:wall') then
       return true
      end
   end
   return false
end

-- return the building the `blueprint` is contained in
--
--    @param blueprint - the blueprint whose building you're interested in
--

function build_util.get_fbp_for(entity)
   if entity and entity:is_valid() then
      -- is this a fabricator?  if so, finding the blueprint and the project is easy!
      local fc = entity:get_component('stonehearth:fabricator')
      if fc then
         return entity, fc:get_blueprint(), fc:get_project()
      end
      
      -- also works for blueprints.  we will take this path if the user passes a
      -- blueprint in directly.
      local cp = entity:get_component('stonehearth:construction_progress')
      if cp then
         local fabricator_entity = cp:get_fabricator_entity()
         if fabricator_entity == entity then
            -- fixture fabricators use the same entity for themselves and the fabricator.
            -- crazy.
            return entity, entity, nil
         end

         if fabricator_entity then
            return build_util.get_fbp_for(fabricator_entity)
         end
         -- no fabricator for this blueprint?  no problem!  just return nil for those
         -- entities.
         return nil, entity, nil
      end

      -- must be a project.  get the fabricator out of the construction data.
      local cd = entity:get_component('stonehearth:construction_data')
      if cd then
         return build_util.get_fbp_for(cd:get_fabricator_entity())
      end
   end
end

function build_util.get_fbp_for_structure(entity, structure_component_name)
   local fabricator, blueprint, project = build_util.get_fbp_for(entity)
   if blueprint then
      local structure_component = blueprint:get_component(structure_component_name)
      if structure_component then
         return fabricator, blueprint, project, structure_component
      end
   end
end


function build_util.get_building_for(entity)
   checks('Entity')

   if entity:get_component('stonehearth:building') then
      return entity
   end

   if entity and entity:is_valid() then
      local _, blueprint, _ = build_util.get_fbp_for(entity)
      if blueprint then
         local cp = blueprint:get_component('stonehearth:construction_progress')
         if cp then
            return cp:get_building_entity()
         end
      end
   end
end

function build_util.get_blueprint_for(entity)
   local fabricator, blueprint, project = build_util.get_fbp_for(entity)
   return blueprint
end

function build_util.can_start_blueprint(blueprint, teardown)
   local building = build_util.get_building_for(blueprint)
   if not building then
      assert(blueprint:get_uri() == 'stonehearth:build:prototypes:scaffolding') -- special case...
      return true
   end
   return building:get_component('stonehearth:building')
                      :can_start_blueprint(blueprint, teardown)
end

function build_util.blueprint_is_finished(blueprint)
   assert(build_util.is_blueprint(blueprint))
   return blueprint:get_component('stonehearth:construction_progress')
                        :get_finished()
end


function build_util.get_cost(building)
   local costs = {
      resources = {},
      items = {},
   }
   radiant.entities.for_all_children(building, function(entity)
         local cp = entity:get_component('stonehearth:construction_progress')
         if cp and entity:get_uri() ~= 'stonehearth:build:prototypes:scaffolding' then
            local fabricator = cp:get_fabricator_component()
            if fabricator then
               fabricator:accumulate_costs(costs)
            end
         end
      end)
   return costs
end

function build_util.save_template(building, header, overwrite)
   local existing_templates = build_util.get_templates()
   local namebase = header.name and header.name:lower() or 'untitled'
   local template_name = namebase

   if not overwrite then
      local count = 1
      while existing_templates[template_name] do
         template_name = namebase .. '_' .. tostring(count)
         count = count + 1
      end
   end
   
   local building_template = save_all_structures_to_template(building)
   building_template.id = building:get_id()

   header.cost = build_util.get_cost(building)

   local template = {
      header = header,
      root = building_template,
   }
   radiant.mods.write_object('building_templates/' .. template_name, template)
end

function build_util.restore_template(building, template_name, options)
   local template = radiant.mods.read_object('building_templates/' .. template_name)
   if not template then
      return
   end

   local rotation = options.rotation
   
   local function rotate_entity(entity)
      for name, _ in pairs(SAVED_COMPONENTS) do
         local component = entity:get_component(name)
         if component and component.rotate_structure then
            component:rotate_structure(rotation)
         end
      end
      for_each_child(entity, function(id, child)
            rotate_entity(child)
         end)
   end

   local entity_map = create_template_entities(building, template)
   load_all_structures_from_template(building, template.root, options, entity_map)

   if rotation and rotation ~= 0 then
      rotate_entity(building)
   end

   building:get_component('stonehearth:building')
               :finish_restoring_template()

   --[[
   if options.mode ~= 'preview' then
      local roof
      local bc = building:get_component('stonehearth:building')
      local structures = bc:get_all_structures()
      local _, entry = next(structures['stonehearth:roof'])
      if entry then
         roof = entry.entity
      end      
      bc:layout_roof(roof)
   end
   ]]
end

function build_util.get_templates()
   local templates = radiant.mods.enum_objects('building_templates')

   local result = {}
   for _, name in ipairs(templates) do
      local template = radiant.mods.read_object('building_templates/' .. name)
      result[name] = template.header
   end

   return result
end

function build_util.get_building_centroid(building)
   local bounds = build_util.get_building_bounds(building)
   local centroid = bounds.min + bounds:get_size():scaled(0.5):to_int();
   centroid.y = 0
   return centroid
end

function build_util.get_building_bounds(building)
   local bounds
   local origin = radiant.entities.local_to_world(Point3.zero, building)
   assert(origin)

   local function measure_bounds(entity)
      if build_util.is_blueprint(entity) then
         local dst = entity:get_component('destination')
         if not dst then
            dst = entity:get_component('region_collision_shape')
         end
         if dst then
            local region_bounds = dst:get_region():get():get_bounds()
            region_bounds = radiant.entities.local_to_world(region_bounds, entity)
                                                :translated(-origin)
            if bounds then
               bounds:grow(region_bounds)
            else
               bounds = region_bounds
            end
         end
      end
      
      for_each_child(entity, function(id, child)
            measure_bounds(child, bounds)
         end)
   end
   measure_bounds(building)
   return bounds
end

function build_util.rotate_structure(entity, degrees)
   local destination = entity:get_component('destination')
   if destination then
      destination:get_region():modify(function(cursor)
            local origin = Point3(0.5, 0, 0.5)
            cursor:translate(-origin)
            cursor:rotate(degrees)
            cursor:translate(origin)
         end)
   end

   local mob = entity:get_component('mob')
   local rotated = mob:get_location()
                           :rotated(degrees)
   mob:move_to(rotated)
end

function build_util.pack_entity_table(tbl)
   return radiant.keys(tbl)
end

function build_util.unpack_entity_table(tbl, entity_map)
   local unpacked = {}
   for _, id in ipairs(tbl) do
      local entity = entity_map[id]
      assert(entity)
      unpacked[entity:get_id()] = entity
   end
   return unpacked
end

function build_util.pack_entity(entity)
   return entity and entity:get_id() or nil
end

function build_util.unpack_entity(id, entity_map)
   local entity
   if id then
      entity = entity_map[id]
   end
   return entity
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
--
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

-- just like edge_point_to_point, but don't move the points out in the
-- xz plane.  instead, move the up 1 in the y direction
--
local function edge_point_to_point_inset(edge_point)
   local point = Point3(edge_point.location.x, 1, edge_point.location.y)
   if edge_point.accumulated_normals.x > 0 then
      point.x = point.x - edge_point.accumulated_normals.x
   end
   if edge_point.accumulated_normals.y > 0 then
      point.z = point.z - edge_point.accumulated_normals.y
   end
   return point
end

function build_util.get_footprint_region2(blueprint, query_point)
   checks('Entity', '?Point3')
   assert(build_util.is_blueprint(blueprint))

   local region = blueprint:get_component('destination')
                                    :get_region()
                                       :get()

   local y = query_point and query_point.y or 0

   -- take just the floor we have at this level
   local slice = Cube3(Point3(-INFINITE, y,     -INFINITE),
                       Point3( INFINITE, y + 1,  INFINITE))
   local slice_region = region:intersect_cube(slice)

   -- calculate the local footprint of the floor.
   local footprint = Region2()
   for cube in slice_region:each_cube() do
      local rect = Rect2(Point2(cube.min.x, cube.min.z),
                         Point2(cube.max.x, cube.max.z),
                         0)
      footprint:add_cube(rect)
   end

   if y > 0 then
      -- subtract out the floor on the next level
      slice:translate(Point3.unit_y)
      local next_slice_region = region:intersect_cube(slice)
      for cube in next_slice_region:each_cube() do
         local rect = Rect2(Point2(cube.min.x, cube.min.z),
                            Point2(cube.max.x, cube.max.z),
                            0)
         footprint:subtract_cube(rect)
      end
   end

   -- force optimizing here usually isn't necessary for solid shapes and
   -- is too expensive for complex ones.  so let it rock!

   return footprint
end

function build_util.get_footprint_region3(blueprint)
   local region = blueprint:get_component('destination')
                                    :get_region()
                                       :get()

   local base = region:get_bounds().min.y - 1

   -- calculate the local footprint of the floor.
   local footprint = Region3()
   for cube in region:each_cube() do
      local rect = Cube3(Point3(cube.min.x, base,     cube.min.z),
                         Point3(cube.max.x, base + 1, cube.max.z))
      footprint:add_cube(rect)
   end
   return footprint
end

function build_util.grow_walls_around(floor, query_point, visitor_fn)
   checks('Entity', '?Point3', 'function')

   -- could be a fabricator...
   floor = build_util.get_blueprint_for(floor)

   local dy = query_point and query_point.y or 0
   local footprint = build_util.get_footprint_region2(floor, query_point)

   -- figure out where the columns and walls should go in world space
   local building = build_util.get_building_for(floor)
   local floor_origin = radiant.entities.get_world_grid_location(floor)
   local wall_origin = floor_origin + Point3(0, dy, 0)
   local building_origin = radiant.entities.get_world_grid_location(building)

   -- if the floor is below the origin of the building, make sure the walls and
   -- columns are not!   
   local height = wall_origin.y - building_origin.y
   if height < 0 then
      wall_origin.y = wall_origin.y - height
   end

   -- if the floor is on the 2nd storey or higher, we grow walls in an 'inset'
   -- pattern.  instead of drawing the walls around the floor, we draw them on
   -- top of it.
   local ep2p = height > 0 and edge_point_to_point_inset or edge_point_to_point
   
   -- convert each 2d edge to 3d min and max coordinates and add a wall span
   -- for each one.
   local edges = footprint:get_edge_list()
   for edge in edges:each_edge() do
      local min = ep2p(edge.min) + wall_origin
      local max = ep2p(edge.max) + wall_origin
      local normal = Point3(edge.normal.x, 0, edge.normal.y)

      if min ~= max then
         visitor_fn(min, max, normal)
      end
   end
end

local function convert_point_for_edge_loop(column, bounds)
   checks('Entity', '?Rect2')

   local pt = radiant.entities.get_world_grid_location(column)
   local normal = column:get_component('stonehearth:column')
                           :get_accumulated_normal()

   local point = { x = pt.x, y = pt.z }
   if normal.x > 0 then
      point.x = point.x + 1
   end
   if normal.z > 0 then
      point.y = point.y + 1
   end

   if bounds then
      if bounds.min.x == INFINITE then
         bounds.min.x = point.x
         bounds.min.y = point.y
         bounds.max.x = point.x
         bounds.max.y = point.y
      else
         bounds.min.x = math.min(bounds.min.x, point.x)
         bounds.min.y = math.min(bounds.min.y, point.y)
         bounds.max.x = math.max(bounds.max.x, point.x)
         bounds.max.y = math.max(bounds.max.y, point.y)
      end
   end
   return point
end

function build_util.create_edge_loop_for_wall(wall, edges, walls, bounds)
   local id = wall:get_id()
   if walls[id] then
      return
   end   
   walls[id] = wall
   local wc = wall:get_component('stonehearth:wall')
   if wc then
      local col_a, col_b = wc:get_columns()
      if col_a then
         local normal = wc:get_normal()
         local edge = {
            min     = convert_point_for_edge_loop(col_a, bounds),
            max     = convert_point_for_edge_loop(col_b, bounds),
            normal  = { x = normal.x, y = normal.y }
         }
         assert(edge.min.x <= edge.max.x and edge.min.y <= edge.max.y)
         table.insert(edges, edge)
      end
      local columns = { wc:get_columns() }
      for _, col in pairs(columns) do
         local connected_walls = col:get_component('stonehearth:column')
                                       :get_connected_walls()
         for _, connected_wall in pairs(connected_walls) do
            build_util.create_edge_loop_for_wall(connected_wall, edges, walls, bounds)
         end
      end
   end
end

function build_util.edge_loop_to_region2(edges, bounds)   
   local function accumulate_region(region, volume)
      -- xor volume into region
      local sub_region = region:intersect_cube(volume)
      local add_region = Region2(volume) - sub_region
      region:add_region(add_region)
      region:subtract_region(sub_region)
   end

   local all_regions = {
      Region2(),     -- left
      Region2(),     -- right
      Region2(),     -- bottom
      Region2(),     -- top
   }
   local cursor = Point2()

   -- edges are of the form:
   -- {
   --    { min = { 1, 1 }, max = { 2, 1}, normal = { 0, 1 }}
   --     ...
   -- }
   local normals = {
      { x = -1, y =  0 },
      { x =  1, y =  0 },
      { x =  0, y = -1 },
      { x =  0, y =  1 },
   }
   for i, normal in pairs(normals) do
      local accum_region = all_regions[i]
      for _, edge in pairs(edges) do
         -- create a volume for this edge in the opposite direction of the normal
         if normal.x == 0 and edge.normal.x == 0 or
            normal.y == 0 and edge.normal.y == 0 then

            local volume = Rect2(Point2(edge.min.x, edge.min.y),
                                 Point2(edge.max.x, edge.max.y));

            if normal.x == -1 then
               volume.max.x = bounds.max.x
            elseif normal.x == 1 then
               volume.min.x = bounds.min.x
            elseif normal.y == -1 then
               volume.max.y = bounds.max.y
            elseif normal.y == 1 then
               volume.min.y = bounds.min.y
            end
            assert(volume.min.x <= volume.max.x and volume.min.y <= volume.max.y)

            accumulate_region(accum_region, volume)
         end
      end
   end

   local region2 = Region2(bounds)
   for _, region in pairs(all_regions) do
      region2 = region2:intersect_region(region)
   end
   return region2
end

function build_util.calculate_roof_shape_around_walls(root_wall, options)
   local last_edge_count, new_edge_count = 0, 0
   local edges, walls = {}, {}
   local bounds = Rect2(Point2(INFINITE, INFINITE), Point2(INFINITE, INFINITE))
   local region2 = Region2()

   local root_wall_origin = radiant.entities.get_world_grid_location(root_wall)
   local height = root_wall_origin.y + constants.STOREY_HEIGHT

   local root_wall_blueprint = build_util.get_blueprint_for(root_wall)
   local root_building = build_util.get_building_for(root_wall_blueprint)

   build_util.create_edge_loop_for_wall(root_wall_blueprint, edges, walls, bounds)

   local world_region2
   repeat
      world_region2 = build_util.edge_loop_to_region2(edges, bounds)
      -- great!  this is the region 2.  now we need to see if there are any walls
      -- inside this region which weren't added to the loop.
      local query_region = Region3()
      for rect in world_region2:each_cube() do
         assert(rect.min.x <= rect.max.x)
         assert(rect.min.y <= rect.max.y)
         local c = Cube3(Point3(rect.min.x, height - 1, rect.min.y),
                         Point3(rect.max.x, height,     rect.max.y))
         query_region:add_unique_cube(c)
      end
      local new_walls = radiant.terrain.get_entities_in_region(query_region, function(entity)
            return build_util.is_blueprint(entity, 'stonehearth:wall')
         end)

      last_edge_count = #edges
      for _, wall in pairs(new_walls) do
         build_util.create_edge_loop_for_wall(wall, edges, walls, bounds)
      end
      new_edge_count = #edges
   until last_edge_count == new_edge_count

   local world_region2_bounds = world_region2:get_bounds()
      
   -- stencil out parts of the region which overlap existing blueprints.  this lets
   -- us grow the "rest" of the roof on a lower storey after having authored all the
   -- upper stories (e.g. a church with a steeple)
   local to_remove = Region2()
   for rect in world_region2:each_cube() do
      local query_cube = Cube3(Point3(rect.min.x, height,     rect.min.y),
                               Point3(rect.max.x, height + 1, rect.max.y))
      radiant.terrain.get_entities_in_cube(query_cube, function(entity)
            if not build_util.is_blueprint(entity) then
               return
            end
            if build_util.get_building_for(entity) == root_building then
               local entity_origin = radiant.entities.get_world_grid_location(entity)
               local rgn = entity:get_component('destination')
                                    :get_region()
                                       :get()
                                          :translated(entity_origin)
               for c in rgn:each_cube() do
                  if c.min.y <= height and c.max.y > height then
                     local rect = Rect2(Point2(c.min.x, c.min.z),
                                        Point2(c.max.x, c.max.z))
                     to_remove:add_cube(rect)
                  end
               end
            end
         end)
   end
   world_region2:subtract_region(to_remove)

   -- return the origin, local region, and all the walls we're growing around
   local region_origin = world_region2_bounds.min
   local region2 = world_region2:translated(-region_origin)
   local world_origin = Point3(region_origin.x,
                               height,
                               region_origin.y)
                 
   return world_origin, region2, walls
end


function build_util.can_grow_roof_around_walls(walls)
   local columns = {}
   for _, wall in pairs(walls) do
      local wc = wall:get_component('stonehearth:wall')
      local roof = wc:get_roof()
      if roof then
         return false, 'stonehearth:build:error:already_connected', wall
      end

      for _, column in pairs({ wc:get_columns() }) do
         local id = column:get_id()
         if not columns[id] then
            columns[id] = column
            roof = column:get_component('stonehearth:column')
                              :get_roof()
            if roof then
               return false, 'stonehearth:build:error:already_connected', column
            end
         end
      end
   end
   return true, columns
end

function build_util.grow_local_box_to_roof(roof, entity, local_box)
   checks('Entity', 'Entity', 'Cube3')


   local roof_origin = radiant.entities.get_world_grid_location(roof)
   local roof_region = roof:get_component('destination')
                              :get_region()
                                 :get()
                                    :translated(roof_origin)

   local origin = radiant.entities.get_world_grid_location(entity)
   assert(origin)


   local clipper = Cube3(local_box.min, local_box.max)
   clipper.min.y = local_box.max.y
   clipper.max.y = INFINITE
   clipper:translate(origin)

   local p0, p1 = local_box.min + origin, local_box.max + origin
   local stencil = Cube3(Point3(p0.x, p1.y, p0.z),
                         Point3(p1.x, INFINITE, p1.z))
   
   -- clip out the part above us..
   local roof_overhang = roof_region:clipped(stencil)
  
   -- iterate through the overhang and merge shingle that are atop each
   -- other
   local merged_roof_overhang = Region3()

   roof_overhang:force_optimize_by_defragmentation('grow local box to roof')
   for shingle in roof_overhang:each_cube() do
      local merged = Cube3(Point3(shingle.min.x, shingle.min.y, shingle.min.z),
                           Point3(shingle.max.x, INFINITE,      shingle.max.z))
      merged_roof_overhang:add_cube(merged)
   end

   local shape = Region3(local_box:translated(origin))

   -- add the shape between the top of the wall and the bottom of the merged overhang
   -- to the local box.
   local merged_bounds = merged_roof_overhang:get_bounds()
   local x0 = math.max(merged_bounds.min.x, p0.x)
   local x1 = math.min(merged_bounds.max.x, p1.x)
   local z0 = math.max(merged_bounds.min.z, p0.z)
   local z1 = math.min(merged_bounds.max.z, p1.z)
   if x1 > x0 and z1 > z0 then
      local grow_bounds = Cube3(Point3(x0, p1.y, z0), Point3(x1, INFINITE, z1))
      local grow_region = Region3(grow_bounds) - merged_roof_overhang
      shape:add_region(grow_region)
   end   
   
   -- translate back into local coordinates before returning
   return shape:translated(-origin)
end

function build_util.bind_fabricator_to_blueprint(blueprint, fabricator, fabricator_component_name)
   local fabricator_component = fabricator:get_component(fabricator_component_name)
   assert(fabricator_component)
   
   -- get the building for this blueprint.  everyone must have a building by now, except
   -- scaffolding.  scaffolding parts are managed by the scaffolding_manager.
   local building = build_util.get_building_for(blueprint)
   if not building then
      assert(blueprint:get_uri() == 'stonehearth:build:prototypes:scaffolding')
   end
   
   local project = fabricator_component:get_project()
   if project then
      local cd_component = project:get_component('stonehearth:construction_data')
      if cd_component then
         if building then
            cd_component:set_building_entity(building)
         end
         cd_component:set_fabricator_entity(fabricator)
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
   if building then
      building:get_component('stonehearth:building')
                  :add_structure(blueprint)
   end
end

-- removes all regions from entities `entity` depends on from `region`
--
local function clip_dependant_regions_from_recursive(building, blueprint, region_origin, region, visited)
   local id = blueprint:get_id()
   if visited[id] then
      return
   end
   blueprint[id] = true

   local bc = building:get_component('stonehearth:building')
   
   for _, dep in bc:each_dependency(blueprint) do
      local dep_origin = radiant.entities.get_world_grid_location(dep)
      local offset = dep_origin - region_origin
      local dep_region = dep:get_component('destination')
                              :get_region()
                                 :get()
                                    :translated(offset)
      region:subtract_region(dep_region)

      clip_dependant_regions_from_recursive(building, dep, region_origin, region, visited)
   end
end

-- removes all regions from entities `entity` depends on from `region`
--
function build_util.clip_dependant_regions_from(entity, region_origin, region)
   checks('Entity', 'Point3', 'Region3')

   local blueprint = build_util.get_blueprint_for(entity)
   if not blueprint then
      return
   end
   local building = build_util.get_building_for(entity)
   if not building then
      return
   end
   clip_dependant_regions_from_recursive(building, blueprint, region_origin, region, {})
end

function build_util.get_blueprint_world_region(blueprint, origin)
   checks('Entity', '?Point3')
   assert(build_util.is_blueprint(blueprint))

   if not origin then
      origin = radiant.get_world_grid_location(blueprint)
   end
   local dst = blueprint:get_component('destination')
   if not dst then
      return
   end
   local rgn = dst:get_region()
   if not rgn then
      return
   end
   return rgn:get():translated(origin)
end

return build_util
