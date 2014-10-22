local constants = require('constants').construction
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local Quaternion = _radiant.csg.Quaternion

local build_util = {}

local SAVED_COMPONENTS = {
   ['stonehearth:wall'] = true,
   ['stonehearth:column'] = true,
   ['stonehearth:roof'] = true,
   ['stonehearth:floor'] = true,
   ['stonehearth:building'] = true,
   ['stonehearth:fixture_fabricator'] = true,      -- for placed item locations
   ['stonehearth:construction_data'] = true,       -- for nine grid info
   ['stonehearth:construction_progress'] = true,   -- for dependencies
   ['stonehearth:no_construction_zone'] = true,    -- for footprint
}

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
               :set_material('materials/place_template_preview.xml')
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
   local faction = radiant.entities.get_faction(building)

   entity_map[tonumber(template.root.id)] = building
   
   local function create_entities(children)
      if children then
         for orig_id, o in pairs(children) do
            local entity = radiant.entities.create_entity(o.uri)
            entity:add_component('unit_info')
                     :set_player_id(player_id)
                     :set_faction(faction)

            entity_map[tonumber(orig_id)] = entity
            create_entities(o.children)
         end
      end
   end
   create_entities(template.root.children)
   return entity_map
end

local function save_structure_to_template(entity, template)
   local template = {}
   local uri = entity:get_uri()
   template.uri = uri

   if uri ~= 'stonehearth:entities:building' then
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
      not build_util.is_building(entity) and
      not build_util.is_footprint(entity) then
      return nil
   end

   local template = save_structure_to_template(entity)

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
-- have stonehearth:construction_progress components.
--
--    @param entity - the entity to be tested for blueprintedness
--
function build_util.is_blueprint(entity)
   return entity:get_component('stonehearth:construction_progress') ~= nil
end

function build_util.is_building(entity)
   return entity:get_component('stonehearth:building') ~= nil
end

function build_util.is_fabricator(entity)
   return entity:get_component('stonehearth:fabricator') ~= nil or
          entity:get_component('stonehearth:fixture_fabricator') ~= nil
end

function build_util.is_footprint(entity)
   return entity:get_component('stonehearth:no_construction_zone') ~= nil
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

   -- intentionally synchronous trigger
   radiant.events.trigger(entity_map, 'finished_loading')

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

function build_util.get_building_bounds(building)
   local bounds = Cube3(Point3.zero, Point3.zero, 0)
   local function measure_bounds(entity)
      local dst = entity:get_component('region_collision_shape')
      if dst then
         bounds:grow(dst:get_region():get():get_bounds())
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
      unpacked[id] = entity
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

return build_util
