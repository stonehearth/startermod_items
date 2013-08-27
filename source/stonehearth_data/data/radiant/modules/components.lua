local components = {}
local singleton = {}

function components.__init()
end

components.__init()
return components

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
function components.create_root_objects()
   self._root_entity = singleton.create_entity()
   assert(self._root_entity:get_id() == 1)

   self._clock = components.add_component(self._root_entity, 'clock')   
   local container = components.add_component(self._root_entity, 'entity_container')
   local build_orders = components.add_component(self._root_entity, 'build_orders')
   --local render_grid = components.add_component(self._root_entity, 'render_grid')
   --local collision_shape = components.add_component(self._root_entity, 'grid_collision_shape')
   
   self._terrain = components.add_component(self._root_entity, 'terrain')
   self._terrain_container_id = container:get_id()
   --self._terrain_grid = native:create_grid()
   --terrain:set_grid(self._terrain_grid)
   --render_grid:set_grid(self._terrain_grid)
   --collision_shape:set_grid(self._terrain_grid)       
end

function components.get_terrain()
   return self._terrain
end

function components.get_root_entity()
   return self._root_entity
end

function components.place_on_terrain(entity, arg1, arg2, arg3)
   radiant.check.is_entity(entity)
   
   local location
   if type(arg1) == 'number' then
      location = Point3(arg1, arg2, arg3)
   else
      location = arg1
   end   
   radiant.check.is_a(location, Point3)
   
   singleton.add_child_to_entity(self._root_entity, entity, location)
   components.add_component(entity, 'render_info'):set_display_iconic(true);
   singleton.get_terrain():place_entity(entity, location)
end

function components.remove_from_terrain(entity)
   radiant.check.is_entity(entity)
   
   singleton.remove_child_from_entity(self._root_entity, entity)
end

function components.add_blueprint(blueprint)
   radiant.check.is_entity(blueprint)
   singleton.get_build_orders():add_blueprint(blueprint)
end

function components.start_project(blueprint)
   radiant.check.is_entity(blueprint)
   
   local project = singleton.create_entity()
   singleton.get_build_orders():start_project(blueprint, project)  
   singleton.place_on_terrain(project, singleton.get_grid_location(blueprint))
   
   return project
end

function components.get_build_orders()
   return components.get_component(self._root_entity, 'build_orders')   
end

function components.increment_clock(interval)
   local now = singleton.now() + interval
   self._clock:set_time(now)
   return now
end

function components.now()
   return self._clock:get_time()
end

function components.create_entity(kind)
   assert(env:is_running())
   assert(not kind or type(kind) == 'string')
   
   local entity = native:create_entity(kind and kind or '')
   log:info('creating new entity %d (kind: %s)', entity:get_id(), kind and kind or '-empty-')
   
   if kind then
      singleton._init_entity(entity, kind);
   end
   
   return entity
end

function components.get_entity(id)
   radiant.check.is_number(id)
   local entity = native:get_entity(id)
   return entity and entity:is_valid() and entity or nil
end

function components.on_destroy_entity(entity, dtor)
   radiant.check.is_entity(entity)
   local id = entity:get_id()
   if not self._entity_dtors[id] then
      self._entity_dtors[id] = {}
   end
   table.insert(self._entity_dtors[id], dtor)
end

function components.destroy_entity(entity)
   radiant.check.is_entity(entity)
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

function components._init_entity(entity, kind)

   -- add standard components, based on the kind of entity we want
   local resource = native:lookup_resource(kind)
   if resource then
      local obj = dkjson.decode(resource:get_json())
      singleton._add_msg_handlers(entity, obj)
      singleton._add_ai(entity, obj)
      singleton._add_animation(entity, obj)
   end
end

function components._add_msg_handlers(entity, obj)
   if obj.scripts then
      for _, name in ipairs(obj.scripts) do
         log:info("adding msg handler %s.", name);
         md:add_msg_handler(entity, name, obj)
      end
   end
end

function components._add_ai(entity, obj)
   if obj.ai then
      ai_mgr:init_entity(entity, obj.ai)
   end
end

function components._add_animation(entity, obj)
   if obj.animation_table then
      if not ani_mgr then
         ani_mgr = require 'radiant.core.animation'
      end
      ani_mgr:get_animation(entity, obj)
   end
end

function components.add_rig_to_entity(entity, rig)
   radiant.check.is_entity(entity)

   if type(rig) ~= 'string' then
      radiant.check.is_a(rig, RigResource);
      rig = rig:get_resource_identifier()
   end
   radiant.check.is_string(rig);
   log:debug('adding rig "%s" to entity %d.', rig, entity:get_id())

   local rig_component = components.add_component(entity, 'render_rig')
   rig_component:add_rig(rig)
end

function components.remove_rig_from_entity(entity, rig)
   radiant.check.is_entity(entity)

   if type(rig) ~= 'string' then
      radiant.check.is_a(rig, RigResource);
      rig = rig:get_resource_identifier()
   end
   radiant.check.is_string(rig);
   log:debug('removing rig "%s" from entity %d.', rig, entity:get_id())

   local rig_component = components.add_component(entity, 'render_rig')
   rig_component:remove_rig(rig)
end

function components.set_display_name(entity, name)
   radiant.check.is_entity(entity)
   radiant.check.is_string(name)

   local component = components.add_component(entity, 'unit_info')
   radiant.check.is_a(component, UnitInfo)
   
   component:set_display_name(name)
end

function components.set_description(entity, name)
   radiant.check.is_entity(entity)
   radiant.check.is_string(name)

   local component = components.add_component(entity, 'unit_info')
   radiant.check.is_a(component, UnitInfo)
   
   component:set_description(name)
end

function components.get_faction(entity)
   radiant.check.is_entity(entity)

   if not components.has_component(entity, 'unit_info') then
      return nil
   end      
   return components.get_component(entity, 'unit_info'):get_faction()
end

function components.get_display_name(entity)
   radiant.check.is_entity(entity)

   local component = components.get_component(entity, 'unit_info')
   radiant.check.is_a(component, UnitInfo)
   
   return component:get_display_name()
end

function components.get_location(entity)
   radiant.check.is_entity(entity)
   return components.add_component(entity, 'mob'):get_location()
end

function components.get_grid_location(entity)
   radiant.check.is_entity(entity)
   return components.add_component(entity, 'mob'):get_grid_location()
end

function components.get_world_grid_location(entity)
   radiant.check.is_entity(entity)
   return components.add_component(entity, 'mob'):get_world_grid_location()
end

function components.get_world_location(entity)
   radiant.check.is_entity(entity)
   return components.add_component(entity, 'mob'):get_world_location()
end

function components.can_teardown_structure(worker, structure)
   return singleton.worker_can_pickup_material(worker, structure:get_material(), 1)
end

function components.worker_can_pickup_material(worker, material, count)
   radiant.check.is_entity(worker)
   radiant.check.is_string(material)
   radiant.check.is_number(count)
   
   local carry_block = components.get_component(worker, 'carry_block') 
   if not carry_block then
      return false
   end
   
   if not carry_block:is_carrying() then
      -- Not carrying anything.  Good to go!
      return true
   end
   
   local item = components.get_component(carry_block:get_carrying(), 'item')
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

function components.teardown(worker, structure)   
   radiant.check.verify(singleton.can_teardown_structure(worker, structure))
   
   local carry_block = components.get_component(worker, 'carry_block')
   if carry_block:is_carrying() then
      local item = components.get_component(carry_block:get_carrying(), 'item')
      item:set_stacks(item:get_stacks() + 1)
   else
      local item = singleton.create_entity('/stonehearth/resources/oak_tree/oak_log')
      components.get_component(item, 'item'):set_stacks(1)
      carry_block:set_carrying(item)
   end
end

function components.get_carrying(entity)
   radiant.check.is_entity(entity)
   
   local carry_block = components.get_component(entity, 'carry_block') 
   if not carry_block then
      return nil
   end
  
   if not carry_block:is_carrying() then
      return nil
   end
   
   return carry_block:get_carrying()
end


function components.pickup_item(entity, item)
   radiant.check.is_entity(entity)
   radiant.check.is_entity(item)
   
   local carry_block = components.get_component(entity, 'carry_block') 
   radiant.check.verify(carry_block ~= nil)

   if item then
      singleton.remove_from_terrain(item)
      carry_block:set_carrying(item)
      singleton.move_to(item, Point3(0, 0, 0))
   else
      carry_block:set_carrying(nil)
   end
end

function components.drop_carrying(entity, location)
   radiant.check.is_entity(entity)
   radiant.check.is_a(location, Point3)

   local carry_block = components.get_component(entity, 'carry_block') 
   if carry_block then
      local item = carry_block:get_carrying()
      if item then
         carry_block:set_carrying(nil)
         singleton.place_on_terrain(item, location)
      end
   end
end

function components.equip(entity, slot, item)
   radiant.check.is_entity(entity)
   radiant.check.is_number(slot)
   radiant.check.verify(slot >= 0 and slot < Paperdoll.NUM_SLOTS)
   
   local paperdoll = components.add_component(entity, 'paperdoll')
   if util:is_entity(item) then
      paperdoll:equip(slot, item)
   else
      paperdoll:unequip(slot)
   end
end

function components.get_equipped(entity, slot)
   radiant.check.is_entity(entity)
   radiant.check.is_number(slot)
   local paperdoll = components.get_component(entity, 'paperdoll')
   if paperdoll and paperdoll:has_item_in_slot(slot) then
      return paperdoll:get_item_in_slot(slot)
   end
end

function components.on_removed_from_terrain(entity, cb)
   local mob = components.get_component(entity, 'mob')
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

function components.get_sight_sensor(entity)
   local sensor = singleton.get_sensor(entity, 'sight')
   if not sensor then
      sensor = singleton.create_sensor(entity, 'sight', 1)
   end
   return sensor
end

function components.create_sensor(entity, name, radius)
   radiant.check.is_entity(entity)
   radiant.check.is_number(radius)
   
   local sensor_list = components.add_component(entity, 'sensor_list')
   local sensor = sensor_list:get_sensor(name)
   radiant.check.verify(not sensor)

   return sensor_list:add_sensor(name, radius)   
end

function components.destroy_sensor(entity, name)
   radiant.check.is_string(name)
   local sensor_list = components.get_component(entity, 'sensor_list')
   if sensor_list then
      sensor_list:remove_sensor(name)
   end
end

function components.get_sensor(entity, name)
   radiant.check.is_string(name)
   local sensor_list = components.get_component(entity, 'sensor_list')
   if sensor_list then
      return sensor_list:get_sensor(name)
   end
end

function components.get_attribute(entity, name)
   radiant.check.is_entity(entity)
   local attributes = components.get_component(entity, 'attributes')
   if not attributes then
      return 0, false
   end
   return attributes:get_attribute(name)
end

function components.set_attribute(entity, name, value)
   radiant.check.is_entity(entity)
   radiant.check.is_number(value)
   local attributes = components.get_component(entity, 'attributes')
   assert(attributes)
   attributes:set_attribute(name, value)
end

function components.update_attribute(entity, name, delta)
   radiant.check.is_entity(entity)
   local attributes = components.get_component(entity, 'attributes')
   if not attributes then
      return nil
   end
   local value = attributes:get_attribute(name)
   value = value + delta
   attributes:set_attribute(name, value)
   return value
end

function components.apply_aura(entity, name, source)
   radiant.check.is_entity(entity)
   radiant.check.is_string(name)
   radiant.check.is_entity(source)
   
   local auras = components.add_component(entity, 'aura_list')
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

function components.create_target_table(entity, group)
   radiant.check.is_entity(entity)
   radiant.check.is_string(group)
   
   local target_tables = components.add_component(entity, 'target_tables')
   return target_tables:add_table(group)
end

function components.destroy_target_table(entity, table)
   radiant.check.is_entity(entity)
   radiant.check.is_a(table, TargetTable)
   
   local target_tables = components.add_component(entity, 'target_tables')
   return target_tables:remove_table(table)
end

function components.get_target_table_entry(entity, group, target)
   radiant.check.is_entity(entity)
   radiant.check.is_string(group)
   radiant.check.is_entity(target)
   
   local target_tables = components.add_component(entity, 'target_tables')
   local table = target_tables:add_table(group)
   return table:add_entry(target)
end

function components.get_target_table_top(entity, name)
   local target_tables = components.add_component(entity, 'target_tables')
   return target_tables:get_top(name)
end

function components.get_distance_between(entity_a, entity_b)
   radiant.check.is_entity(entity_a)
   radiant.check.is_entity(entity_b)
   local pos_a = singleton.get_world_grid_location(entity_a)
   local pos_b = singleton.get_world_grid_location(entity_b)
   return pos_a:distance_to(pos_b)
end

function components.is_adjacent_to(arg1, arg2)
   -- xxx: this style doesn't work until we fix util:is_a().
   --local point_a = util:is_a(arg1, Entity) and singleton.get_world_grid_location(arg1) or arg1
   --local point_b = util:is_a(arg2, Entity) and singleton.get_world_grid_location(arg2) or arg2
   local point_a = singleton.get_world_grid_location(arg1)
   local point_b = arg2
   
   radiant.check.is_a(point_a, Point3)
   radiant.check.is_a(point_b, Point3)
   return point_a:is_adjacent_to(point_b)
end

function components.add_to_inventory(entity,  item)
   radiant.check.is_entity(entity)
   radiant.check.is_entity(item)
   
   local inventory = components.add_component(entity, 'inventory')
   if not inventory:is_full() then
      inventory:stash_item(item)
   end
   return false
end

function components.get_movement_info(entity, travel_mode)
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

function components.wear(entity, clothing)
   radiant.check.is_entity(entity)
   radiant.check.is_entity(clothing)
   
   local clothing_rigs = components.get_component(clothing, 'render_rig')   
   if clothing_rigs then
      local entity_rigs = components.add_component(entity, 'render_rig')
      for article in clothing_rigs:get_rigs():items() do -- xxx :each()
         entity_rigs:add_rig(article)         
      end
   end
end

function components.is_hostile(faction_a, faction_b)   
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


function components.get_hostile_components(to, sensor)
   local items = {}
   for id in sensor:get_contents():items() do table.insert(items, id) end

   -- xxx: only components on the world should be detected by sensors, right?
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

function components.filter_combat_abilities(entity, filter_fn)
   local result = {}
   local combat_abilities = components.get_component(entity, 'combat_ability_list')
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

function components.add_combat_ability(entity, ability_name)
   local combat_abilities = components.add_component(entity, 'combat_ability_list')
   combat_abilities:add_combat_ability(ability_name)
end

function components.frame_count_to_time(frames)
   return math.floor(frames * 1000 / 30)
end

function components.get_target(entity)
   radiant.check.is_entity(entity)
   local combat_abilities = om:get_component(entity, 'combat_ability_list')
   if combat_abilities then
      return combat_abilities:get_target()
   end
end

function components.set_target(entity, target)
   check:is_entity(entity)
   local combat_abilities = components.add_component(entity, 'combat_ability_list')
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

function components.get_build_order(entity)
   for _, name in ipairs(all_build_orders) do
      if self:has_component(entity, name) then
         return self:get_component(entity, name)
      end
   end
end

return ObjectModel()
]]