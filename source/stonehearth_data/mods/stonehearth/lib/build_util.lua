local constants = require('constants').construction
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local Point2 = _radiant.csg.Point2
local Rect2 = _radiant.csg.Rect2
local Region2 = _radiant.csg.Region2
local Quaternion = _radiant.csg.Quaternion

local build_util = {}

local roation_eps = 0.01

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
      local name = options.mode == 'preview' and 'region_collision_shape' or 'destination'
      local region = radiant.alloc_region3()
      region:modify(function(cursor)
            cursor:load(template.shape)
         end)
      entity:add_component(name)
               :set_region(region)
   end

   if options.mode == 'preview' then
      entity:add_component('render_info')
               :set_material('materials/place_template_preview.json')
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

   local destination = entity:get_component('destination')
   if destination then
      template.shape = destination:get_region():get()
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

function build_util.can_start_building(blueprint)
   local building = build_util.get_building_for(blueprint)
   if not building then
      assert(blueprint:get_uri() == 'stonehearth:build:prototypes:scaffolding') -- special case...
      return true
   end
   return building:get_component('stonehearth:building')
                      :can_start_building(blueprint)
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

function build_util.get_footprint_region2(blueprint)
   local region = blueprint:get_component('destination')
                                    :get_region()
                                       :get()

   -- calculate the local footprint of the floor.
   local footprint = Region2()
   for cube in region:each_cube() do
      local rect = Rect2(Point2(cube.min.x, cube.min.z),
                         Point2(cube.max.x, cube.max.z),
                         0)
      footprint:add_cube(rect)
   end
   footprint:force_optimize_by_merge('creating footprint region')
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

function build_util.grow_walls_around(floor, visitor_fn)
   local footprint = build_util.get_footprint_region2(floor)

   -- figure out where the columns and walls should go in world space
   local building = build_util.get_building_for(floor)
   local floor_origin = radiant.entities.get_world_grid_location(floor)
   local building_origin = radiant.entities.get_world_grid_location(building)

   -- if the floor is below the origin of the building, make sure the walls and
   -- columns are not!   
   local height = floor_origin.y - building_origin.y
   if height < 0 then
      floor_origin.y = floor_origin.y - height
   end

   -- if the floor is on the 2nd storey or higher, we grow walls in an 'inset'
   -- pattern.  instead of drawing the walls around the floor, we draw them on
   -- top of it.
   local ep2p = height > 0 and edge_point_to_point_inset or edge_point_to_point
   
   -- convert each 2d edge to 3d min and max coordinates and add a wall span
   -- for each one.
   local edges = footprint:get_edge_list()
   for edge in edges:each_edge() do
      local min = ep2p(edge.min) + floor_origin
      local max = ep2p(edge.max) + floor_origin
      local normal = Point3(edge.normal.x, 0, edge.normal.y)

      if min ~= max then
         visitor_fn(min, max, normal)
      end
   end
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

return build_util
