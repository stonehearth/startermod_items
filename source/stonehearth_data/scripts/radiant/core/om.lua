require 'unclasslib'
local dkjson = require ("dkjson")
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

ObjectModel.AllComponents = {
   action_list = 'ActionList',
   clock = 'Clock',
   stockpile_designation = 'StockpileDesignation',
   entity_container = 'EntityContainer',
   item = 'Item',
   mob = 'Mob',
   path_jump_node = 'PathJumpNode',
   resource_node = 'ResourceNode',
   render_grid = 'RenderGrid',
   render_rig = 'RenderRig',
   grid_collision_shape = 'GridCollisionShape',
   sphere_collision_shape = 'SphereCollisionShape',
   terrain = 'Terrain',
   carry_block = 'CarryBlock',
   unit_info = 'UnitInfo',
   wall = 'Wall',
   room = 'Room',
   floor = 'Floor',
   scaffolding = 'Scaffolding',
   post = 'Post',
   peaked_roof = 'PeakedRoof',
   portal = 'Portal',
   fixture = 'Fixture',
   build_orders = 'BuildOrders',
   build_order_dependencies = 'BuildOrdersDependencies',
   resource = 'Resource',
   user_behavior_queue = 'UserBehaviorQueue',
   effect_list = 'EffectList',
   render_info = 'RenderInfo',
   profession = 'Profession',
   automation_queue = 'AutomationQueue',
   paperdoll = 'Paperdoll',
   sensor_list = 'SensorList',
   attributes = 'Attributes',
   aura_list = 'AuraList',
   target_tables = 'TargetTables',
   inventory = 'Inventory',
   weapon_info = 'WeaponInfo',
   combat_ability_list = 'CombatAbilityList',
}

function ObjectModel:__init()
   self._root_entity = nil
   self._terrain_grid = nil

   self._entity_dtors = {}
   
   for _, name in ipairs(ObjectModel.AllComponents) do
      for _, op in { 'add', 'get' } do
         self['add_' .. name .. '_component'] = function (self, entity, name)
            check:is_entity(entity)
            check:is_string(name)
            return entity['add_' .. name .. '_component']()
         end
      end
   end
end

function ObjectModel:create_root_objects()
   self._root_entity = self:create_entity()
   assert(self._root_entity:get_id() == 1)

   self._clock = self:add_component(self._root_entity, 'clock')   
   local container = self:add_component(self._root_entity, 'entity_container')
   local build_orders = self:add_component(self._root_entity, 'build_orders')
   --local render_grid = self:add_component(self._root_entity, 'render_grid')
   --local collision_shape = self:add_component(self._root_entity, 'grid_collision_shape')
   
   self._terrain = self:add_component(self._root_entity, 'terrain')
   self._terrain_container_id = container:get_id()
   --self._terrain_grid = native:create_grid()
   --terrain:set_grid(self._terrain_grid)
   --render_grid:set_grid(self._terrain_grid)
   --collision_shape:set_grid(self._terrain_grid)       
end

function ObjectModel:get_terrain()
   return self._terrain
end

function ObjectModel:get_root_entity()
   return self._root_entity
end

function ObjectModel:place_on_terrain(entity, arg1, arg2, arg3)
   check:is_entity(entity)
   
   local location
   if type(arg1) == 'number' then
      location = Point3(arg1, arg2, arg3)
   else
      location = arg1
   end   
   check:is_a(location, Point3)
   
   self:add_child_to_entity(self._root_entity, entity, location)
   self:add_component(entity, 'render_info'):set_display_iconic(true);
   self:get_terrain():place_entity(entity, location)
end

function ObjectModel:remove_from_terrain(entity)
   check:is_entity(entity)
   
   self:remove_child_from_entity(self._root_entity, entity)
end

function ObjectModel:add_blueprint(blueprint)
   check:is_entity(blueprint)
   self:get_build_orders():add_blueprint(blueprint)
end

function ObjectModel:start_project(blueprint)
   check:is_entity(blueprint)
   
   local project = self:create_entity()
   self:get_build_orders():start_project(blueprint, project)  
   self:place_on_terrain(project, self:get_grid_location(blueprint))
   
   return project
end

function ObjectModel:get_build_orders()
   return self:get_component(self._root_entity, 'build_orders')   
end

function ObjectModel:increment_clock(interval)
   local now = self:now() + interval
   self._clock:set_time(now)
   return now
end

function ObjectModel:now()
   return self._clock:get_time()
end

function ObjectModel:create_entity(kind)
   assert(env:is_running())
   assert(not kind or type(kind) == 'string')
   
   local entity = native:create_entity(kind and kind or '')
   log:info('creating new entity %d (kind: %s)', entity:get_id(), kind and kind or '-empty-')
   
   if kind then
      self:_init_entity(entity, kind);
   end
   
   return entity
end

function ObjectModel:get_entity(id)
   check:is_number(id)
   local entity = native:get_entity(id)
   return entity and entity:is_valid() and entity or nil
end

function ObjectModel:on_destroy_entity(entity, dtor)
   check:is_entity(entity)
   local id = entity:get_id()
   if not self._entity_dtors[id] then
      self._entity_dtors[id] = {}
   end
   table.insert(self._entity_dtors[id], dtor)
end

function ObjectModel:destroy_entity(entity)
   check:is_entity(entity)
   local id = entity:get_id()
   local dtors = self._entity_dtors[id]
   if dtors then
      for _, dtor in ipairs(dtors) do
         dtor()
      end
      self._entity_dtors[id] = nil
   end
   native:destroy_entity(entity)
end

function ObjectModel:get_component(entity, name)
   if name == 'transform' then
      name = 'mob'
   end
   check:is_entity(entity)
   check:verify(ObjectModel.AllComponents[name])
   if self:has_component(entity, name) then
      return entity['get_'..name..'_component'](entity)
   end   
end

function ObjectModel:has_component(entity, name)
   check:is_entity(entity)
   check:verify(ObjectModel.AllComponents[name])
   return entity['has_'..name..'_component'](entity)
end

function ObjectModel:add_component(entity, name)
   check:is_entity(entity)
   check:verify(ObjectModel.AllComponents[name])
   return entity['add_'..name..'_component'](entity)
end

--[[
function ObjectModel:start_animation(entity, sequence, start_time, end_time, flags)
   check:is_entity(entity)
   check:is_string(sequence)
   check:is_number(start_time)
   check:is_number(end_time)
   check:verify(flags > 0 or end_time >= start_time)
   check:is_number(flags)
   
   local rig_component = self:get_component(entity, 'render_rig')
   if not rig_component then
      check:report_error('Entity %d has no rig.  Can\'t start animation.', entity:get_id())
      return
   end
   rig_component:start_animation(sequence, start_time, end_time, flags)
   
   local duration = (end_time > 0) and tostring(end_time) or 'forever'
   --log:info('entity %d starting animation %s (start:%d until:%s).', entity:get_id(), sequence, start_time, duration)
end
]]

function ObjectModel:_init_entity(entity, kind)

   -- add standard components, based on the kind of entity we want
   local resource = native:lookup_resource(kind)
   if resource then
      local obj = dkjson.decode(resource:get_json())
      self:_add_msg_handlers(entity, obj)
      self:_add_ai(entity, obj)
      self:_add_animation(entity, obj)
   end
end

function ObjectModel:_add_msg_handlers(entity, obj)
   if obj.scripts then
      for _, name in ipairs(obj.scripts) do
         log:info("adding msg handler %s.", name);
         md:add_msg_handler(entity, name, obj)
      end
   end
end

function ObjectModel:_add_ai(entity, obj)
   if obj.ai then
      ai_mgr:init_entity(entity, obj.ai)
   end
end

function ObjectModel:_add_animation(entity, obj)
   if obj.animation_table then
      if not ani_mgr then
         ani_mgr = require 'radiant.core.animation'
      end
      ani_mgr:get_animation(entity, obj)
   end
end

function ObjectModel:add_rig_to_entity(entity, rig)
   check:is_entity(entity)

   if type(rig) ~= 'string' then
      check:is_a(rig, RigResource);
      rig = rig:get_resource_identifier()
   end
   check:is_string(rig);
   log:debug('adding rig "%s" to entity %d.', rig, entity:get_id())

   local rig_component = self:add_component(entity, 'render_rig')
   rig_component:add_rig(rig)
end

function ObjectModel:remove_rig_from_entity(entity, rig)
   check:is_entity(entity)

   if type(rig) ~= 'string' then
      check:is_a(rig, RigResource);
      rig = rig:get_resource_identifier()
   end
   check:is_string(rig);
   log:debug('removing rig "%s" from entity %d.', rig, entity:get_id())

   local rig_component = self:add_component(entity, 'render_rig')
   rig_component:remove_rig(rig)
end

function ObjectModel:set_display_name(entity, name)
   check:is_entity(entity)
   check:is_string(name)

   local component = self:add_component(entity, 'unit_info')
   check:is_a(component, UnitInfo)
   
   component:set_display_name(name)
end

function ObjectModel:set_description(entity, name)
   check:is_entity(entity)
   check:is_string(name)

   local component = self:add_component(entity, 'unit_info')
   check:is_a(component, UnitInfo)
   
   component:set_description(name)
end

function ObjectModel:get_faction(entity)
   check:is_entity(entity)

   if not self:has_component(entity, 'unit_info') then
      return nil
   end      
   return self:get_component(entity, 'unit_info'):get_faction()
end

function ObjectModel:get_display_name(entity)
   check:is_entity(entity)

   local component = self:get_component(entity, 'unit_info')
   check:is_a(component, UnitInfo)
   
   return component:get_display_name()
end


function ObjectModel:add_child_to_entity(parent, child, location)
   check:is_entity(parent)
   check:is_entity(child)

   local component = self:get_component(parent, 'entity_container')
   check:is_a(component, EntityContainer)

   component:add_child(child)
   self:move_entity_to(child, location)
end

function ObjectModel:remove_child_from_entity(parent, child)
   check:is_entity(parent)
   check:is_entity(child)

   local component = self:get_component(parent, 'entity_container')
   check:is_a(component, EntityContainer)

   component:remove_child(child)
   self:move_entity_to(child, Point3(0, 0, 0))
end

function ObjectModel:move_entity_to(entity, location)
   check:is_entity(entity)
   check:is_a(location, Point3)
   self:add_component(entity, 'mob'):set_location_grid_aligned(location)
end

function ObjectModel:turn_to(entity, degrees)
   check:is_entity(entity)
   check:is_number(degrees)
   self:add_component(entity, 'mob'):turn_to(degrees)
end

function ObjectModel:turn_to_face(entity, arg2)
   --local location = util:is_a(arg2, Entity) and self:get_world_grid_location(arg2) or arg2
   local location = arg2

   check:is_entity(entity)
   check:is_a(location, Point3)
   self:add_component(entity, 'mob'):turn_to_face_point(location)
end

function ObjectModel:get_location(entity)
   check:is_entity(entity)
   return self:add_component(entity, 'mob'):get_location()
end

function ObjectModel:get_grid_location(entity)
   check:is_entity(entity)
   return self:add_component(entity, 'mob'):get_grid_location()
end

function ObjectModel:get_world_grid_location(entity)
   check:is_entity(entity)
   return self:add_component(entity, 'mob'):get_world_grid_location()
end

function ObjectModel:get_world_location(entity)
   check:is_entity(entity)
   return self:add_component(entity, 'mob'):get_world_location()
end

function ObjectModel:can_teardown_structure(worker, structure)
   return self:worker_can_pickup_material(worker, structure:get_material(), 1)
end

function ObjectModel:worker_can_pickup_material(worker, material, count)
   check:is_entity(worker)
   check:is_string(material)
   check:is_number(count)
   
   local carry_block = self:get_component(worker, 'carry_block') 
   if not carry_block then
      return false
   end
   
   if not carry_block:is_carrying() then
      -- Not carrying anything.  Good to go!
      return true
   end
   
   local item = self:get_component(carry_block:get_carrying(), 'item')
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

function ObjectModel:teardown(worker, structure)   
   check:verify(self:can_teardown_structure(worker, structure))
   
   local carry_block = self:get_component(worker, 'carry_block')
   if carry_block:is_carrying() then
      local item = self:get_component(carry_block:get_carrying(), 'item')
      item:set_stacks(item:get_stacks() + 1)
   else
      local item = self:create_entity('module://stonehearth/resources/oak_tree/oak_log')
      self:get_component(item, 'item'):set_stacks(1)
      carry_block:set_carrying(item)
   end
end

function ObjectModel:get_carrying(entity)
   check:is_entity(entity)
   
   local carry_block = self:get_component(entity, 'carry_block') 
   if not carry_block then
      return nil
   end
  
   if not carry_block:is_carrying() then
      return nil
   end
   
   return carry_block:get_carrying()
end


function ObjectModel:pickup_item(entity, item)
   check:is_entity(entity)
   check:is_entity(item)
   
   local carry_block = self:get_component(entity, 'carry_block') 
   check:verify(carry_block ~= nil)

   if item then
      self:remove_from_terrain(item)
      carry_block:set_carrying(item)
      self:move_entity_to(item, Point3(0, 0, 0))
   else
      carry_block:set_carrying(nil)
   end
end

function ObjectModel:drop_carrying(entity, location)
   check:is_entity(entity)
   check:is_a(location, Point3)

   local carry_block = self:get_component(entity, 'carry_block') 
   if carry_block then
      local item = carry_block:get_carrying()
      if item then
         carry_block:set_carrying(nil)
         self:place_on_terrain(item, location)
      end
   end
end

function ObjectModel:equip(entity, slot, item)
   check:is_entity(entity)
   check:is_number(slot)
   check:verify(slot >= 0 and slot < Paperdoll.NUM_SLOTS)
   
   local paperdoll = self:add_component(entity, 'paperdoll')
   if util:is_entity(item) then
      paperdoll:equip(slot, item)
   else
      paperdoll:unequip(slot)
   end
end

function ObjectModel:get_equipped(entity, slot)
   check:is_entity(entity)
   check:is_number(slot)
   local paperdoll = self:get_component(entity, 'paperdoll')
   if paperdoll and paperdoll:has_item_in_slot(slot) then
      return paperdoll:get_item_in_slot(slot)
   end
end

function ObjectModel:on_removed_from_terrain(entity, cb)
   local mob = self:get_component(entity, 'mob')
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

function ObjectModel:get_sight_sensor(entity)
   local sensor = self:get_sensor(entity, 'sight')
   if not sensor then
      sensor = self:create_sensor(entity, 'sight', 1)
   end
   return sensor
end

function ObjectModel:create_sensor(entity, name, radius)
   check:is_entity(entity)
   check:is_number(radius)
   
   local sensor_list = self:add_component(entity, 'sensor_list')
   local sensor = sensor_list:get_sensor(name)
   check:verify(not sensor)

   return sensor_list:add_sensor(name, radius)   
end

function ObjectModel:destroy_sensor(entity, name)
   check:is_string(name)
   local sensor_list = self:get_component(entity, 'sensor_list')
   if sensor_list then
      sensor_list:remove_sensor(name)
   end
end

function ObjectModel:get_sensor(entity, name)
   check:is_string(name)
   local sensor_list = self:get_component(entity, 'sensor_list')
   if sensor_list then
      return sensor_list:get_sensor(name)
   end
end

function ObjectModel:get_attribute(entity, name)
   check:is_entity(entity)
   local attributes = self:get_component(entity, 'attributes')
   if not attributes then
      return 0, false
   end
   return attributes:get_attribute(name)
end

function ObjectModel:set_attribute(entity, name, value)
   check:is_entity(entity)
   check:is_number(value)
   local attributes = self:get_component(entity, 'attributes')
   assert(attributes)
   attributes:set_attribute(name, value)
end

function ObjectModel:update_attribute(entity, name, delta)
   check:is_entity(entity)
   local attributes = self:get_component(entity, 'attributes')
   if not attributes then
      return nil
   end
   local value = attributes:get_attribute(name)
   value = value + delta
   attributes:set_attribute(name, value)
   return value
end

function ObjectModel:apply_aura(entity, name, source)
   check:is_entity(entity)
   check:is_string(name)
   check:is_entity(source)
   
   local auras = self:add_component(entity, 'aura_list')
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

function ObjectModel:create_target_table(entity, group)
   check:is_entity(entity)
   check:is_string(group)
   
   local target_tables = self:add_component(entity, 'target_tables')
   return target_tables:add_table(group)
end

function ObjectModel:destroy_target_table(entity, table)
   check:is_entity(entity)
   check:is_a(table, TargetTable)
   
   local target_tables = self:add_component(entity, 'target_tables')
   return target_tables:remove_table(table)
end

function ObjectModel:get_target_table_entry(entity, group, target)
   check:is_entity(entity)
   check:is_string(group)
   check:is_entity(target)
   
   local target_tables = self:add_component(entity, 'target_tables')
   local table = target_tables:add_table(group)
   return table:add_entry(target)
end

function ObjectModel:get_target_table_top(entity, name)
   local target_tables = self:add_component(entity, 'target_tables')
   return target_tables:get_top(name)
end

function ObjectModel:get_distance_between(entity_a, entity_b)
   check:is_entity(entity_a)
   check:is_entity(entity_b)
   local pos_a = self:get_world_grid_location(entity_a)
   local pos_b = self:get_world_grid_location(entity_b)
   return pos_a:distance_to(pos_b)
end

function ObjectModel:is_adjacent_to(arg1, arg2)
   -- xxx: this style doesn't work until we fix util:is_a().
   --local point_a = util:is_a(arg1, Entity) and self:get_world_grid_location(arg1) or arg1
   --local point_b = util:is_a(arg2, Entity) and self:get_world_grid_location(arg2) or arg2
   local point_a = self:get_world_grid_location(arg1)
   local point_b = arg2
   
   check:is_a(point_a, Point3)
   check:is_a(point_b, Point3)
   return point_a:is_adjacent_to(point_b)
end

function ObjectModel:add_to_inventory(entity,  item)
   check:is_entity(entity)
   check:is_entity(item)
   
   local inventory = self:add_component(entity, 'inventory')
   if not inventory:is_full() then
      inventory:stash_item(item)
   end
   return false
end

function ObjectModel:get_movement_info(entity, travel_mode)
   check:is_entity(entity)
   
   -- xxx: this is all horrible....
   travel_mode = travel_mode and travel_mode or 'run'
   if self:get_carrying(entity) then
      if travel_mode == 'run' then
         travel_mode = 'carry_walk'
      else
         travel_mode = "carry_" .. travel_mode
      end
   end
   local speed, success = self:get_attribute(entity, travel_mode .. "_travel_speed")
   if not success then
      speed = ObjectModel.DefaultTravelSpeeds[travel_mode]
   end
   assert(speed)
   return {
      effect_name = travel_mode,
      speed = speed
   }
end

function ObjectModel:wear(entity, clothing)
   check:is_entity(entity)
   check:is_entity(clothing)
   
   local clothing_rigs = self:get_component(clothing, 'render_rig')   
   if clothing_rigs then
      local entity_rigs = self:add_component(entity, 'render_rig')
      for article in clothing_rigs:get_rigs():items() do -- xxx :each()
         entity_rigs:add_rig(article)         
      end
   end
end

function ObjectModel:is_hostile(faction_a, faction_b)   
   if util:is_a(faction_a, Entity) then
      check:verify(faction_a:is_valid())
      faction_a = self:get_faction(faction_a)
   end
   if util:is_a(faction_b, Entity) then
      check:verify(faction_b:is_valid())
      if not faction_b:is_valid() then
         return false
      end
      faction_b = self:get_faction(faction_b)
   end
   return faction_a and faction_b and faction_a ~= faction_b
end


function ObjectModel:get_hostile_entities(to, sensor)
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
         local entity = self:get_entity(id)
         if util:is_entity(entity) and self:is_hostile(to, entity) then
            return entity
         end
      end
   end
end

function ObjectModel:filter_combat_abilities(entity, filter_fn)
   local result = {}
   local combat_abilities = self:get_component(entity, 'combat_ability_list')
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

function ObjectModel:add_combat_ability(entity, ability_name)
   local combat_abilities = self:add_component(entity, 'combat_ability_list')
   combat_abilities:add_combat_ability(ability_name)
end

function ObjectModel:frame_count_to_time(frames)
   return math.floor(frames * 1000 / 30)
end

function ObjectModel:get_target(entity)
   check:is_entity(entity)
   local combat_abilities = om:get_component(entity, 'combat_ability_list')
   if combat_abilities then
      return combat_abilities:get_target()
   end
end

function ObjectModel:set_target(entity, target)
   check:is_entity(entity)
   local combat_abilities = self:add_component(entity, 'combat_ability_list')
   assert(combat_abilities)
   if combat_abilities then
      if util:is_entity(target) then
         combat_abilities:set_target(target)
      else
         combat_abilities:clear_target()
      end
   end
end

function ObjectModel:get_build_order(entity)
   for _, name in ipairs(all_build_orders) do
      if self:has_component(entity, name) then
         return self:get_component(entity, name)
      end
   end
end

return ObjectModel()
