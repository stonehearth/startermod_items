local entities = {}
local singleton = {}

local Point3 = _radiant.csg.Point3

function entities.__init()
   singleton._entity_dtors = {}
end

function entities._add_scripts(entity, obj)
   if obj.scripts then
      for _, name in ipairs(obj.scripts) do
         -- the correct way to do this is to have the entity loader
         -- attach scripts directly... not by name!!
         radiant.log.info("adding msg handler %s.", name);

         local ctor = radiant.resources.lua_require(name)
         radiant.events.add_msg_handler(entity, ctor)
      end
   end
end

function entities._init_entity(entity, uri)
   -- add standard components, based on the kind of entity we want
   local obj = radiant.resources.load_json(uri)

   if obj then
      if obj.components then
         for name, json in radiant.resources.pairs(obj.components) do
            assert(json)
            local component = entity:add_component(name)
            if component and component.extend then
               if type(component) == 'userdata' then
                  json = radiant.resources.get_native_obj(json)
               end
               component:extend(json)
            end
         end
      end
      --entities._add_scripts(entity, obj)
   end
end

--[[
Opposite of _init_entity. Given a uri, remove
component influences from the entity.
--]]
function entities.unregister_from_entity(entity, uri)
   local obj = radiant.resources.load_json(uri)

   if obj then
      if obj.components then
         for name, json in radiant.resources.pairs(obj.components) do
            assert(json)
            if name == 'radiant:ai' then
               radiant.ai.unregister(entity, json)
            end
            --[[
            -- How to do this generically? Full remove component?
            local component = entity:get_component(name)
            --Until then, call unregister on whatever components have it.
            -- add standard components, based on the kind of entity we want
            if component and component.unregister or  radiant.mods.load_api(name) and  then
               component:unregister(entity, json)
            end
            if component and component.extend then
               --TODO: implement an un-extend?
            end
            --]]
         end
      end
   end
end

function entities.get_root_entity()
   return radiant._root_entity
end

function entities.create_entity(uri)
   assert(radiant.gamestate.is_initialized())
   assert(not uri or type(uri) == 'string')

   local entity = native:create_entity()
   radiant.log.info('creating new entity %d (uri: %s)', entity:get_id(), uri and uri or '-empty-')
   entity:set_debug_name(uri and uri or '-unknown-')

   if uri then
      entity:set_resource_uri(uri)
      entities._init_entity(entity, uri);
   end
   return entity
end

function entities.destroy_entity(entity)
   radiant.check.is_entity(entity)
   local id = entity:get_id()
   local dtors = singleton._entity_dtors[id]
   if dtors then
      for _, dtor in ipairs(dtors) do
         dtor()
      end
      singleton._entity_dtors[id] = nil
   end
   native:destroy_entity(entity)
end

function entities.inject_into_entity(entity, uri)
   entities._init_entity(entity, uri)
end

function entities.add_child(parent, child, location)
   radiant.check.is_entity(parent)
   radiant.check.is_entity(child)

   local component = parent:add_component('entity_container')

   component:add_child(child)
   entities.move_to(child, location)
end

function entities.remove_child(parent, child)
   radiant.check.is_entity(parent)
   radiant.check.is_entity(child)

   local component = parent:get_component('entity_container')

   component:remove_child(child)
   entities.move_to(child, Point3(0, 0, 0))
end

function entities.move_to(entity, location)
   radiant.check.is_entity(entity)

   if type(location) == "table" then
      location = Point3(location.x, location.y, location.z)
   end
   entity:add_component('mob'):set_location_grid_aligned(location)
end

function entities.turn_to(entity, degrees)
   radiant.check.is_entity(entity)
   radiant.check.is_number(degrees)
   entity:add_component('mob'):turn_to(degrees)
end

function entities.turn_to_face(entity, arg2)
   --local location = util:is_a(arg2, Entity) and singleton.get_world_grid_location(arg2) or arg2
   local location
   if arg2.x == nil then
      location = arg2:get_component('mob'):get_world_grid_location()
   else
      location = arg2
   end

   radiant.check.is_entity(entity)
   entity:add_component('mob'):turn_to_face_point(location)
end

function entities.get_location_aligned(entity)
   radiant.check.is_entity(entity)
   return entity:add_component('mob'):get_grid_location()
end

function entities.get_world_grid_location(entity)
   radiant.check.is_entity(entity)
   return entity:add_component('mob'):get_world_grid_location()
end

function entities.on_destroy(entity, dtor)
   radiant.check.is_entity(entity)
   local id = entity:get_id()
   if not singleton._entity_dtors[id] then
      singleton._entity_dtors[id] = {}
   end
   table.insert(singleton._entity_dtors[id], dtor)
end

function entities.add_outfit(entity, outfit_uri)
   radiant.check.is_entity(entity)
   radiant.check.is_string(outfit_uri)

   radiant.log.debug('adding outfit "%s" to entity %d.', outfit_uri, entity:get_id())

   local rig_component = entity:add_component('render_rig')
   rig_component:add_rig(outfit_uri)
end

function entities.create_target_table(entity, group)
   radiant.check.is_entity(entity)
   radiant.check.is_string(group)

   local target_tables = entity:add_component('target_tables')
   return target_tables:add_table(group)
end

function entities.destroy_target_table(entity, table)
   radiant.check.is_entity(entity)

   local target_tables = entity:add_component('target_tables')
   return target_tables:remove_table(table)
end

--[[
   Gets carried item
   returns: entity that is being carried, nil otherwise
]]
function entities.get_carrying(entity)
   local carry_block = entity:get_component('carry_block')
   if not carry_block then
      return nil
   end
   return carry_block:is_carrying() and carry_block:get_carrying() or nil
end

--[[
   Is this entity carrying something?
   returns: nil if it can't carry, false if it's not carrying, and
            both true AND a reference to the carry block
            if it is carrying something.
]]
function entities.is_carrying(entity)
   radiant.check.is_entity(entity)

   local carry_block = entity:get_component('carry_block')
   if not carry_block then
      return nil
   end

   return carry_block:is_carrying()
end

function entities.do_ability(entity, ability_name, ...)
   radiant.check.is_entity(entity)
   radiant.check.is_string(ability_name)
   local abilities = entity:get_component('ability_list')
   if abilities then
      ability = abilities:get_ability()
      local api = radiant.mods.load_api(mod)
      if api and api[ability_name] then
         return api[ability_name](...)
      end
   end
end

--Get an entity given its numeric ID
function entities.get_entity(id)
   radiant.check.is_number(id)
   return native:get_entity(id)
end

--Get an entity given its string ID (from client,
--something like "/object/31"
function entities.get_entity_by_string(string_id)
   local numeric_part = string.sub(string_id, 9)
   return entities.get_entity(tonumber(numeric_part))
end

function entities.get_animation_table_name(entity)
   local name
   local render_info = entity:get_component('render_info')
   if render_info then
      name = render_info:get_animation_table_name()
   end
   return name
end

--[[
local dkjson = require 'lib.dkjson'
local log = require 'radiant.core.log'
local util = require 'radiant.core.util'
local check = require 'radiant.core.check'
local md = require 'radiant.core.md'
local ai_mgr = require 'radiant.ai.ai_mgr'
local ani_mgr

-- xxx: break this up into the init-time and the run-time reactor?
-- perhaps by making the run-time part a singleton??

local ObjectModel = class()


ObjectModel.DefaultTravelSpeeds = {
   run = 1,
   walk = 0.5,
   hop = 0.7,
   carry_walk = 0.5,
   patrol = 0.25,
}



function entities.__init()
   self._root_entity = nil
   self._terrain_grid = nil

   self._entity_dtors = {}

   for _, name in ipairs(ObjectModel.AllComponents) do
      for _, op in { 'add', 'get' } do
         self['add_' .. name .. '_component'] = function (self, entity, name)
            radiant.check.is_entity(entity)
            radiant.check.is_string(name)
            return entity['add_' .. name .. '_component']()
         end
      end
   end
end

function entities.create_root_objects()
   self._root_entity = singleton.create_entity()
   assert(self._root_entity:get_id() == 1)

   self._clock = radiant.components.add_component(self._root_entity, 'clock')
   local container = radiant.components.add_component(self._root_entity, 'entity_container')
   local build_orders = radiant.components.add_component(self._root_entity, 'build_orders')
   --local render_grid = radiant.components.add_component(self._root_entity, 'render_grid')
   --local collision_shape = radiant.components.add_component(self._root_entity, 'grid_collision_shape')

   self._terrain = radiant.components.add_component(self._root_entity, 'terrain')
   self._terrain_container_id = container:get_id()
   --self._terrain_grid = native:create_grid()
   --terrain:set_grid(self._terrain_grid)
   --render_grid:set_grid(self._terrain_grid)
   --collision_shape:set_grid(self._terrain_grid)
end

function entities.get_terrain()
   return self._terrain
end

function entities.get_root_entity()
   return self._root_entity
end

function entities.place_on_terrain(entity, arg1, arg2, arg3)
   radiant.check.is_entity(entity)

   local location
   if type(arg1) == 'number' then
      location = Point3(arg1, arg2, arg3)
   else
      location = arg1
   end
   radiant.check.is_a(location, Point3)

   singleton.add_child_to_entity(self._root_entity, entity, location)
   --entity:add_component('render_info'):set_display_iconic(true);
   singleton.get_terrain():place_entity(entity, location)
end

function entities.remove_from_terrain(entity)
   radiant.check.is_entity(entity)

   singleton.remove_child_from_entity(self._root_entity, entity)
end

function entities.add_blueprint(blueprint)
   radiant.check.is_entity(blueprint)
   singleton.get_build_orders():add_blueprint(blueprint)
end

function entities.start_project(blueprint)
   radiant.check.is_entity(blueprint)

   local project = singleton.create_entity()
   singleton.get_build_orders():start_project(blueprint, project)
   singleton.place_on_terrain(project, singleton.get_grid_location(blueprint))

   return project
end

function entities.get_build_orders()
   return radiant.components.get_component(self._root_entity, 'build_orders')
end

function entities.increment_clock(interval)
   local now = singleton.now() + interval
   self._clock:set_time(now)
   return now
end

function entities.now()
   return self._clock:get_time()
end

function entities.create_entity(kind)
   assert(env:is_running())
   assert(not kind or type(kind) == 'string')

   local entity = native:create_entity(kind and kind or '')
   radiant.log.info('creating new entity %d (kind: %s)', entity:get_id(), kind and kind or '-empty-')

   if kind then
      singleton._init_entity(entity, kind);
   end

   return entity
end

function entities.get_component(entity, name)
   if name == 'transform' then
      name = 'mob'
   end
   radiant.check.is_entity(entity)
   radiant.check.verify(ObjectModel.AllComponents[name])
   if singleton.has_component(entity, name) then
      return entity['get_'..name..'_component'](entity)
   end
end

function entities.has_component(entity, name)
   radiant.check.is_entity(entity)
   radiant.check.verify(ObjectModel.AllComponents[name])
   return entity['has_'..name..'_component'](entity)
end

function entities.add_component(entity, name)
   radiant.check.is_entity(entity)
   radiant.check.verify(ObjectModel.AllComponents[name])
   return entity['add_'..name..'_component'](entity)
end

function entities._init_entity(entity, kind)

   -- add standard components, based on the kind of entity we want
   local resource = native:lookup_resource(kind)
   if resource then
      local obj = dkjson.decode(resource:get_json())
      singleton._add_msg_handlers(entity, obj)
      singleton._add_ai(entity, obj)
      singleton._add_animation(entity, obj)
   end
end

function entities._add_msg_handlers(entity, obj)
   if obj.scripts then
      for _, name in ipairs(obj.scripts) do
         radiant.log.info("adding msg handler %s.", name);
         md:add_msg_handler(entity, name, obj)
      end
   end
end

function entities._add_ai(entity, obj)
   if obj.ai then
      ai_mgr:init_entity(entity, obj.ai)
   end
end

function entities._add_animation(entity, obj)
   if obj.animation_table then
      if not ani_mgr then
         ani_mgr = require 'radiant.core.animation'
      end
      ani_mgr:get_animation(entity, obj)
   end
end

function entities.set_display_name(entity, name)
   radiant.check.is_entity(entity)
   radiant.check.is_string(name)

   local component = entity:add_component('unit_info')
   radiant.check.is_a(component, UnitInfo)

   component:set_display_name(name)
end

function entities.set_description(entity, name)
   radiant.check.is_entity(entity)
   radiant.check.is_string(name)

   local component = entity:add_component('unit_info')
   radiant.check.is_a(component, UnitInfo)

   component:set_description(name)
end

function entities.get_faction(entity)
   radiant.check.is_entity(entity)

   if not singleton.has_component(entity, 'unit_info') then
      return nil
   end
   return entity:get_component('unit_info'):get_faction()
end

function entities.get_display_name(entity)
   radiant.check.is_entity(entity)

   local component = entity:get_component('unit_info')
   radiant.check.is_a(component, UnitInfo)

   return component:get_display_name()
end

function entities.get_location(entity)
   radiant.check.is_entity(entity)
   return entity:add_component('mob'):get_location()
end

function entities.get_grid_location(entity)
   radiant.check.is_entity(entity)
   return entity:add_component('mob'):get_grid_location()
end

function entities.get_world_grid_location(entity)
   radiant.check.is_entity(entity)
   return entity:add_component('mob'):get_world_grid_location()
end

function entities.get_world_location(entity)
   radiant.check.is_entity(entity)
   return entity:add_component('mob'):get_world_location()
end

function entities.can_teardown_structure(worker, structure)
   return singleton.worker_can_pickup_material(worker, structure:get_material(), 1)
end

function entities.worker_can_pickup_material(worker, material, count)
   radiant.check.is_entity(worker)
   radiant.check.is_string(material)
   radiant.check.is_number(count)

   local carry_block = worker:get_component('carry_block')
   if not carry_block then
      return false
   end

   if not carry_block:is_carrying() then
      -- Not carrying anything.  Good to go!
      return true
   end

   local item = radiant.components.get_component(carry_block:get_carrying(), 'item')
   if not item then
      -- Not carrying an item.  Couldn't possibly be a resource, then.
      return false -- this shouldn't be possible, right?
   end

   if material ~= item:get_material() then
      -- Wrong item...
      return false
   end

   local stacks = item:get_stacks()
   if stacks + count >= item:get_max_stacks() then
      -- Item stacks are full.  Can't grab anymore!
      return false
   end
   return true
end

function entities.teardown(worker, structure)
   radiant.check.verify(singleton.can_teardown_structure(worker, structure))

   local carry_block = worker:get_component('carry_block')
   if carry_block:is_carrying() then
      local item = radiant.components.get_component(carry_block:get_carrying(), 'item')
      item:set_stacks(item:get_stacks() + 1)
   else
      local item = singleton.create_entity('/stonehearth/resources/oak_tree/oak_log')
      item:get_component('item'):set_stacks(1)
      carry_block:set_carrying(item)
   end
end
--]]

--[[
   Tell the entity (a mob, probably) to pick up the item
   entity: probably a mob
   item:   the thing to pick up
]]
function entities.pickup_item(entity, item, parent)
   radiant.check.is_entity(entity)
   radiant.check.is_entity(item)

   local carry_block = entity:get_component('carry_block')
   radiant.check.verify(carry_block ~= nil)

   if item then
      if not parent then
         parent = radiant._root_entity
      end
      entities.remove_child(parent, item)
      carry_block:set_carrying(item)
      entities.move_to(item, Point3(0, 0, 0))
   else
      carry_block:set_carrying(nil)
   end
end

--[[
   Tell the entity (a mob, probably) to drop
   whatever it's carrying at a certain location
   TODO: doesn't the target location have to match the put down animation? Revisit
   entity: probably a mob
   location: the place where we want to put the item
]]
function entities.drop_carrying(entity, location)
   radiant.check.is_entity(entity)
   radiant.check.is_a(location, Point3)

   local carry_block = entity:get_component('carry_block')
   if carry_block then
      local item = carry_block:get_carrying()
      if item then
         local loc = radiant.entities.get_world_grid_location(item)
         carry_block:set_carrying(nil)
         radiant.terrain.place_entity(item, location)
      end
   end
end

--[[
function entities.equip(entity, slot, item)
   radiant.check.is_entity(entity)
   radiant.check.is_number(slot)
   radiant.check.verify(slot >= 0 and slot < Paperdoll.NUM_SLOTS)

   local paperdoll = entity:add_component('paperdoll')
   if util:is_entity(item) then
      paperdoll:equip(slot, item)
   else
      paperdoll:unequip(slot)
   end
end

function entities.get_equipped(entity, slot)
   radiant.check.is_entity(entity)
   radiant.check.is_number(slot)
   local paperdoll = entity:get_component('paperdoll')
   if paperdoll and paperdoll:has_item_in_slot(slot) then
      return paperdoll:get_item_in_slot(slot)
   end
end

function entities.on_removed_from_terrain(entity, cb)
   local mob = entity:get_component('mob')
   if mob then
      local promise
      promise = mob:trace_parent():changed(function (v)
         if not v or v:get_id() ~= self._terrain_container_id then
            cb()
            promise:destroy()
         end
      end)
      -- xxx: what about item destruction?
   end
end

function entities.get_sight_sensor(entity)
   local sensor = singleton.get_sensor(entity, 'sight')
   if not sensor then
      sensor = singleton.create_sensor(entity, 'sight', 1)
   end
   return sensor
end

function entities.create_sensor(entity, name, radius)
   radiant.check.is_entity(entity)
   radiant.check.is_number(radius)

   local sensor_list = entity:add_component('sensor_list')
   local sensor = sensor_list:get_sensor(name)
   radiant.check.verify(not sensor)

   return sensor_list:add_sensor(name, radius)
end

function entities.destroy_sensor(entity, name)
   radiant.check.is_string(name)
   local sensor_list = entity:get_component('sensor_list')
   if sensor_list then
      sensor_list:remove_sensor(name)
   end
end

function entities.get_sensor(entity, name)
   radiant.check.is_string(name)
   local sensor_list = entity:get_component('sensor_list')
   if sensor_list then
      return sensor_list:get_sensor(name)
   end
end

function entities.get_attribute(entity, name)
   radiant.check.is_entity(entity)
   local attributes = entity:get_component('attributes')
   if not attributes then
      return 0, false
   end
   return attributes:get_attribute(name)
end

function entities.set_attribute(entity, name, value)
   radiant.check.is_entity(entity)
   radiant.check.is_number(value)
   local attributes = entity:get_component('attributes')
   assert(attributes)
   attributes:set_attribute(name, value)
end

function entities.update_attribute(entity, name, delta)
   radiant.check.is_entity(entity)
   local attributes = entity:get_component('attributes')
   if not attributes then
      return nil
   end
   local value = attributes:get_attribute(name)
   value = value + delta
   attributes:set_attribute(name, value)
   return value
end

function entities.apply_aura(entity, name, source)
   radiant.check.is_entity(entity)
   radiant.check.is_string(name)
   radiant.check.is_entity(source)

   local auras = entity:add_component('aura_list')
   local aura = auras:get_aura(name, source)
   if not aura then
      aura = auras:create_aura(name, source)
      local resource = native:lookup_resource(name)
      if resource then
         local behavior_name = resource:get('msg_handler')
         if behavior_name then
            local handler = md:create_msg_handler(behavior_name, entity, aura)
            aura:set_msg_handler(handler)
         end
      end
   end
   return aura
end

function entities.get_target_table_entry(entity, group, target)
   radiant.check.is_entity(entity)
   radiant.check.is_string(group)
   radiant.check.is_entity(target)

   local target_tables = entity:add_component('target_tables')
   local table = target_tables:add_table(group)
   return table:add_entry(target)
end

function entities.get_target_table_top(entity, name)
   local target_tables = entity:add_component('target_tables')
   return target_tables:get_top(name)
end

function entities.get_distance_between(entity_a, entity_b)
   radiant.check.is_entity(entity_a)
   radiant.check.is_entity(entity_b)
   local pos_a = singleton.get_world_grid_location(entity_a)
   local pos_b = singleton.get_world_grid_location(entity_b)
   return pos_a:distance_to(pos_b)
end
--]]

--[[
   Checks if an entity is next to a location, updated
   to use get_world_grid location.
   TODO: Still relevant with Tony's pathfinder?
   entity:     the entity to check
   location:   the target location
   returns:    true if the entity is adjacent to the specified ocation
]]
function entities.is_adjacent_to(entity, location)
   --TODO: can I blow away these comments?
   -- xxx: this style doesn't work until we fix util:is_a().
   --local point_a = util:is_a(arg1, Entity) and singleton.get_world_grid_location(arg1) or arg1
   --local point_b = util:is_a(arg2, Entity) and singleton.get_world_grid_location(arg2) or arg2
   --local point_a = singleton.get_world_grid_location(entity)
   local point_a = entities.get_world_grid_location(entity)
   local point_b = location

   radiant.check.is_a(point_a, Point3)
   radiant.check.is_a(point_b, Point3)
   return point_a:is_adjacent_to(point_b)
end

--[[
function entities.add_to_inventory(entity,  item)
   radiant.check.is_entity(entity)
   radiant.check.is_entity(item)

   local inventory = entity:add_component('inventory')
   if not inventory:is_full() then
      inventory:stash_item(item)
   end
   return false
end

function entities.get_movement_info(entity, travel_mode)
   radiant.check.is_entity(entity)

   -- xxx: this is all horrible....
   travel_mode = travel_mode and travel_mode or 'run'
   if singleton.get_carrying(entity) then
      if travel_mode == 'run' then
         travel_mode = 'carry_walk'
      else
         travel_mode = "carry_" .. travel_mode
      end
   end
   local speed, success = singleton.get_attribute(entity, travel_mode .. "_travel_speed")
   if not success then
      speed = ObjectModel.DefaultTravelSpeeds[travel_mode]
   end
   assert(speed)
   return {
      effect_name = travel_mode,
      speed = speed
   }
end

function entities.wear(entity, clothing)
   radiant.check.is_entity(entity)
   radiant.check.is_entity(clothing)

   local clothing_rigs = clothing:get_component('render_rig')
   if clothing_rigs then
      local entity_rigs = entity:add_component('render_rig')
      for article in clothing_rigs:get_rigs():items() do -- xxx :each()
         entity_rigs:add_rig(article)
      end
   end
end

function entities.is_hostile(faction_a, faction_b)
   if util:is_a(faction_a, Entity) then
      radiant.check.verify(faction_a:is_valid())
      faction_a = singleton.get_faction(faction_a)
   end
   if util:is_a(faction_b, Entity) then
      radiant.check.verify(faction_b:is_valid())
      if not faction_b:is_valid() then
         return false
      end
      faction_b = singleton.get_faction(faction_b)
   end
   return faction_a and faction_b and faction_a ~= faction_b
end


function entities.get_hostile_entities(to, sensor)
   local items = {}
   for id in sensor:get_contents():items() do table.insert(items, id) end

   -- xxx: only entities on the world should be detected by sensors, right?
   -- not shit people are carrying!

   local index = 1
   return function()
      while util:is_entity(to) do
         local id
         id, index = items[index], index + 1
         if id == nil then
            return nil
         end
         local entity = singleton.get_entity(id)
         if util:is_entity(entity) and singleton.is_hostile(to, entity) then
            return entity
         end
      end
   end
end

function entities.filter_combat_abilities(entity, filter_fn)
   local result = {}
   local combat_abilities = entity:get_component('combat_ability_list')
   if combat_abilities then
      for name, ability in combat_abilities:get_combat_abilities():items() do
         -- xxx: why is ability nil here?
         ability = combat_abilities:get_combat_ability(name)
         if filter_fn(ability) then
            table.insert(result, ability)
         end
      end
   end
   return result
end

function entities.add_combat_ability(entity, ability_name)
   local combat_abilities = entity:add_component('combat_ability_list')
   combat_abilities:add_combat_ability(ability_name)
end

function entities.frame_count_to_time(frames)
   return math.floor(frames * 1000 / 30)
end

function entities.get_target(entity)
   radiant.check.is_entity(entity)
   local combat_abilities = om:get_component(entity, 'combat_ability_list')
   if combat_abilities then
      return combat_abilities:get_target()
   end
end

function entities.set_target(entity, target)
   check:is_entity(entity)
   local combat_abilities = entity:add_component('combat_ability_list')
   assert(combat_abilities)
   if combat_abilities then
      if util:is_entity(target) then
         combat_abilities:set_target(target)
      else
         combat_abilities:clear_target()
      end
   end
end



local all_build_orders = {
   'wall',
   'floor',
   'scaffolding',
   'post',
   'peaked_roof',
   'fixture'
}

function entities.get_build_order(entity)
   for _, name in ipairs(all_build_orders) do
      if self:has_component(entity, name) then
         return self:get_component(entity, name)
      end
   end
end

return ObjectModel()
]]

entities.__init()
return entities
