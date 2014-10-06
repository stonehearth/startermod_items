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
   ['stonehearth:construction_data'] = true,
   ['stonehearth:construction_progress'] = true,
}

local function alloc_region3()
   if _radiant.client then
      return _radiant.client.alloc_region3()
   end
   return _radiant.sim.alloc_region3()
end

local function load_structure_from_template(entity, template, options, entity_map)
   assert(template and template.uri)

   if template.mob then
      local xf = template.mob
      entity:add_component('mob')
               :move_to(Point3(xf.position.x, xf.position.y, xf.position.z))
               :set_rotation(Quaternion(xf.orientation.w, xf.orientation.x, xf.orientation.y, xf.orientation.z))
   end
   
   if template.destination then
      local component_name = options.mode == 'preview' and 'region_collision_shape' or 'destination'
      local dst_region = alloc_region3()

      dst_region:modify(function(cursor)
            cursor:load(template.destination)
         end)

      entity:add_component(component_name)
               :set_region(dst_region)
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
            radiant.log.write('', 0, ' structure %s loading "%s" from template', name, entity)
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
         entity:add_component('entity_container'):add_child(child)
      end
   end
end

local function create_template_entities(building, template)
   local entity_map = {}
   entity_map[tonumber(template.root.id)] = building
   
   local function create_entities(children)
      if children then
         for orig_id, o in pairs(children) do
            entity_map[tonumber(orig_id)] = radiant.entities.create_entity(o.uri)
            create_entities(o.children)
         end
      end
   end
   create_entities(template.root.children)
   return entity_map
end

local function save_structure_to_template(entity, template)
   local template = {}

   template.uri = entity:get_uri()
   template.mob = entity:get_component('mob')
                           :get_transform()

   local destination = entity:get_component('destination')
   if destination then
      template.destination = destination:get_region():get()
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
   if not build_util.is_blueprint(entity) and not build_util.is_building(entity) then
      return nil
   end

   local template = save_structure_to_template(entity)

   local ec = entity:get_component('entity_container')
   if ec then
      for id, child in ec:each_child() do
         local info = save_all_structures_to_template(child)
         if info then
            if not template.children then
               template.children = {}
            end
            template.children[child:get_id()] = info
         end
      end
   end

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
   if template then
      local entity_map = create_template_entities(building, template)
      load_all_structures_from_template(building, template.root, options, entity_map)
   end
end

function build_util.get_templates()
   local templates = radiant.mods.enum_objects('building_templates')

   local result = {}
   for _, name in ipairs(templates) do
      local template = radiant.mods.read_object('building_templates/' .. name)
      result[name] = template.info
   end

   return templates
end


return build_util
