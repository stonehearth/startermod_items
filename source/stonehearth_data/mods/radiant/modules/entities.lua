local FilteredTrace = require 'modules.filtered_trace'
local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local Entity = _radiant.om.Entity
local log = radiant.log.create_logger('entities')
local rng = _radiant.csg.get_default_rng()

local entities = {}

function entities.__init()
end

function entities.get_root_entity()
   return radiant._root_entity
end

function entities.create_entity(ref)
   local entity = _radiant.sim.create_entity(ref or "")
   log:debug('created entity %s', entity)
   return entity
end

function entities.stop_thinking(entity)
   if entity and entity:is_valid() then
      local ai_c = entity:get_component('stonehearth:ai')

      if ai_c then
         ai_c:stop()
      end
   end
end

function entities.exists_in_world(entity)
   return entities.get_world_location(entity) ~= nil
end

function entities.destroy_entity(entity)
   if entity and entity:is_valid() then
      log:debug('destroying entity %s', entity)
      radiant.check.is_entity(entity)

      -- destroy all the children when destorying the parent.  should we do
      -- this from c++?  if not, entities which get destroyed from the cpp
      -- layer won't get this behavior.  maybe that's just impossible (i.e. forbid
      -- it from happening, since the cpp layer knowns nothing about game logic?)
      local ec = entity:get_component('entity_container')
      if ec then
         -- Copy the blueprint's (container's) children into a local var first, because
         -- _set_teardown_recursive could cause the entity container to be invalidated.
         local ec_children = {}
         for id, child in ec:each_child() do
            ec_children[id] = child
         end         
         for id, child in pairs(ec_children) do
            entities.destroy_entity(child)
         end
      end
      --If we're the big one, destroy the little and ghost one
      local entity_forms = entity:get_component('stonehearth:entity_forms')
      if entity_forms then
         local iconic_entity = entity_forms:get_iconic_entity()
         if iconic_entity then
            _radiant.sim.destroy_entity(iconic_entity)
         end
         local ghost_entity = entity_forms:get_ghost_entity()
         if ghost_entity then
            _radiant.sim.destroy_entity(ghost_entity)
         end
      end
      --If we're the little one, call destroy on the big one and exit
      local iconic_component = entity:get_component('stonehearth:iconic_form')
      if iconic_component then
         local full_sized = iconic_component:get_root_entity()
         entities.destroy_entity(full_sized)
         return 
      end

      --If the entity has specified to run an effect, run it.
      local on_destroy = radiant.entities.get_entity_data(entity, 'on_destroy')
      if on_destroy ~= nil and on_destroy.effect ~= nil then
      
         local proxy_entity = radiant.entities.create_proxy_entity('death effect')
         local location = radiant.entities.get_location_aligned(entity)
         radiant.terrain.place_entity_at_exact_location(proxy_entity, location)

         local effect = radiant.effects.run_effect(proxy_entity, on_destroy.effect)
         effect:set_finished_cb(
            function()
               radiant.entities.destroy_entity(proxy_entity)
            end
         )

      end

      _radiant.sim.destroy_entity(entity)
   end
end

-- Use when the entity is being killed in the world
-- Will trigger sync "kill" event so all components can clean up after themselves
function entities.kill_entity(entity)
   if entity and entity:is_valid() then
      log:debug('killing entity %s', entity)
      radiant.check.is_entity(entity)

      --Trigger an event synchronously, so we don't delete the item till all listeners have done their thing
      radiant.events.trigger(entity, 'stonehearth:kill_event')

      --Trigger a more general event, for non-affiliated components
      radiant.events.trigger_async(radiant.entities, 'stonehearth:entity_killed', {
         entity = entity,
         id = entity:get_id(),
         name = radiant.entities.get_display_name(entity),
         player_id = radiant.entities.get_player_id(entity)
      })

      --For now, just call regular destroy on the entity
      --Review Question: Will it ever be the case that calling destroy is insufficient?
      --for example, if we also need to recursively kill child entities? Are we ever
      --going to apply this to marsupials? If you kill an oiliphant, the dudes on its
      --back aren't immediately killed too, they just fall to the ground, right?
      --"It still only counts as one!" --ChrisGimli
      entities.destroy_entity(entity)
   end
end

function entities.create_proxy_entity(debug_text, use_default_adjacent_region)
   assert(type(debug_text) == 'string')

   if use_default_adjacent_region == nil then
      use_default_adjacent_region = false
   end

   local proxy_entity = radiant.entities.create_entity()
   proxy_entity:set_debug_text(debug_text)
   log:debug('created proxy entity %s', proxy_entity)

   if not use_default_adjacent_region then
      -- cache the origin region since we use this a lot
      if entities.origin_region == nil then
         entities.origin_region = _radiant.sim.alloc_region3()
         entities.origin_region:modify(function(region3)
               region3:add_unique_cube(Cube3(Point3(0, 0, 0), Point3(1, 1, 1)))
            end)
      end

      -- make the adjacent the same as the entity location, so that we actually go to the entity location
      local destination = proxy_entity:add_component('destination')
      destination:set_region(entities.origin_region)
      destination:set_adjacent(entities.origin_region)
   end

   return proxy_entity
end

function entities.get_parent(entity)
   local mob = entity:get_component('mob')
   return mob and mob:get_parent()
end

function entities.add_child(parent, child, location)
   radiant.check.is_entity(parent)
   radiant.check.is_entity(child)

   local component = parent:add_component('entity_container')

   -- it's a significant performance gain to move the entity before adding it
   -- to the new container when putting things on the terrain.  it would probably
   -- be a good idea to remove it from its old parent before moving it, too.
   if location then
      entities.move_to(child, location)
   end
   component:add_child(child)
end

function entities.remove_child(parent, child)
   radiant.check.is_entity(parent)
   radiant.check.is_entity(child)

   local component = parent:get_component('entity_container')
   if component then
      component:remove_child(child:get_id())
   end
end

--TODO: we used to be able to query an ec for a child by ID
--like this ec:get(ingredient.item:get_id())
--What happened?
function entities.has_child_by_id(parent, child_id)
   radiant.check.is_entity(parent)

   local component = parent:get_component('entity_container')
   if not component then
      return false
   end
   return component:get_child(child_id) ~= nil
end

function entities.get_name(entity)
   local unit_info = entity:get_component('unit_info')
   return unit_info and unit_info:get_display_name() or nil
end

function entities.get_category(entity)
   local item = entity:get_component('item')
   
   if item and item:get_category() ~= '' then
      return item:get_category()
   end

   local entity_forms = entity:get_component('stonehearth:entity_forms')

   if entity_forms then
      local iconic_entity = entity_forms:get_iconic_entity()
      return radiant.entities.get_category(iconic_entity)
   end

   return 'none'

end

function entities.set_name(entity, name)
   local unit_info = entity:get_component('unit_info')
   if unit_info then
      unit_info:set_display_name(name)
   end
end

function entities.get_description(entity)
   local unit_info = entity:get_component('unit_info')
   return unit_info and unit_info:get_description() or nil
end

function entities.set_description(entity, description)
   local unit_info = entity:get_component('unit_info')
   if unit_info then
      unit_info:set_description(description)
   end
end

function entities.get_player_id(entity)
   local unit_info = entity:get_component('unit_info')
   return unit_info and unit_info:get_player_id() or nil
end

function entities.is_owned_by_player(entity, player_id)
   return entities.get_player_id(entity) == player_id
end

function entities.is_entity(entity)
   if type(entity.get_type_name) == 'function' then
      return entity:get_type_name() == 'class radiant::om::Entity'
   end
   return false
end

function entities.set_player_id(entity, player_id)
   assert(entity, 'no entity passed to set_player_id')
   assert(player_id, 'no player_id passed to set_player_id')
   if entities.is_entity(player_id) then
      player_id = player_id:add_component('unit_info'):get_player_id()
   end
   return entity:add_component('unit_info'):set_player_id(player_id)
end

function entities.get_location_aligned(entity)
   radiant.check.is_entity(entity)
   if entity then
      return entity:add_component('mob'):get_grid_location()
   end
end

-- returns nil if the entity's parent is nil (i.e. it is not placed in the world)
function entities.get_world_location(entity)
   local mob = entity:get_component('mob')
   local location = mob and mob:get_world_location()
   return location
end

-- returns nil if the entity's parent is nil (i.e. it is not placed in the world)
function entities.get_world_grid_location(entity)
   if entity:get_id() == 1 then
      return Point3(0, 0, 0)
   end
   
   local mob = entity:get_component('mob')
   local location = mob and mob:get_world_grid_location()
   return location
end

-- uris are key, value pairs of uri, quantity
function entities.spawn_items(uris, origin, min_radius, max_radius, player_id)
   local items = {}

   for uri, quantity in pairs(uris) do
      for i = 1, quantity do
         local location = radiant.terrain.find_placement_point(origin, min_radius, max_radius)
         local item = radiant.entities.create_entity(uri)

         items[item:get_id()] = item

         if player_id then
            entities.set_player_id(item, player_id)
         end

         radiant.terrain.place_entity(item, location)
      end
   end

   return items
end

function entities.distance_between(object_a, object_b)
   local mob
   assert(object_a and object_b)

   if radiant.util.is_a(object_a, Entity) then
      mob = object_a:get_component('mob')
      object_a = mob and mob:get_world_location()
   end
   if radiant.util.is_a(object_b, Entity) then
      mob = object_b:get_component('mob')
      object_b = mob and mob:get_world_location()
   end

   if not object_a or not object_b then
      return nil
   end
   
   -- xxx: verify a and b are both Point3fs...
   return object_a:distance_to(object_b)
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

function entities.get_facing(entity)
   return entity:add_component('mob'):get_facing()
end

function entities.turn_to_face(entity, arg2)
   local location
   if radiant.util.is_a(arg2, Entity) then
      location = entities.get_world_location(arg2)
   elseif radiant.util.is_a(arg2, Point3) then
      location = arg2
   end

   if location then
      radiant.check.is_entity(entity)
      entity:add_component('mob'):turn_to_face_point(location)
   end
end

function entities.think(entity, uri, priority)
   radiant.check.is_entity(entity)
   entity:add_component('stonehearth:thought_bubble'):set_thought(uri, priority)
end

function entities.unthink(entity, uri)
   radiant.check.is_entity(entity)
   if entity and entity:is_valid() then
      entity:add_component('stonehearth:thought_bubble'):unset_thought(uri)
   end
end

-- Add an existing item to the entity's equipment component
function entities.equip_item(entity, item)
   entity:add_component('stonehearth:equipment')
               :equip_item(item)
end

--Remove the item from the entity's equipment component
function entities.unequip_item(entity, item)
   entity:add_component('stonehearth:equipment')
               :unequip_item(item)
end

function entities.get_equipped_item(entity, slot)
   local equipment_component = entity:get_component('stonehearth:equipment')
   if equipment_component == nil then
      return nil
   end

   local item = equipment_component:get_item_in_slot(slot)
   return item
end

function entities.get_component_data(arg0, key)
   local uri
   if type(arg0) == 'string' then
      uri = arg0
   else
      local entity = arg0
      uri = entity:get_uri()
   end
   if uri and #uri > 0 then
      local json = radiant.resources.load_json(uri)
      if json.components then
         return json.components[key]
      end
   end
end

function entities.get_entity_data(arg0, key)
   local uri
   if type(arg0) == 'string' then
      uri = arg0
   else
      local entity = arg0
      uri = entity:get_uri()
   end
   if uri and #uri > 0 then
      local json = radiant.resources.load_json(uri)
      if json.entity_data then
         return json.entity_data[key]
      end
   end
end

function entities.add_outfit(entity, outfit_uri)
   radiant.check.is_entity(entity)
   radiant.check.is_string(outfit_uri)

   log:debug('adding outfit "%s" to entity %d.', outfit_uri, entity:get_id())

   local rig_component = entity:add_component('render_rig')
   rig_component:add_rig(outfit_uri)
end

--[[
   Gets carried item
   returns: entity that is being carried, nil otherwise
]]
function entities.get_carrying(entity)
   local carry_block = entity:get_component('stonehearth:carry_block')
   if not carry_block then
      return nil
   end
   return carry_block:get_carrying()
end

--[[
   Is this entity carrying something?
   returns: nil if it can't carry, false if it's not carrying, and
            both true AND a reference to the carry block
            if it is carrying something.
]]
function entities.is_carrying(entity)
   radiant.check.is_entity(entity)

   local carry_block = entity:get_component('stonehearth:carry_block')
   if not carry_block then
      return false
   end
   return carry_block:is_carrying()
end

-- id here can be an int (e.g. 999) or uri (e.g. '/o/stores/server/objects/999')
function entities.get_entity(id)
   local entity = _radiant.sim.get_object(id)
   return entity
end


function entities.get_animation_table(entity)
   local name
   local render_info = entity:get_component('render_info')
   if render_info then
      name = render_info:get_animation_table()
   end
   return name
end

function entities.get_display_name(entity)
   radiant.check.is_entity(entity)

   local component = entity:get_component('unit_info')
   if component then
      return component:get_display_name()
   end
end

function entities.set_display_name(entity, name)
   radiant.check.is_entity(entity)
   radiant.check.is_string(name)

   entity:add_component('unit_info')
               :set_display_name(name)
end

function entities.get_attribute(entity, attribute_name)
   if entity and entity:is_valid() then
      return entity:add_component('stonehearth:attributes'):get_attribute(attribute_name)
   end
end


function entities.set_attribute(entity, attribute_name, value)
   if entity and entity:is_valid() then
      entity:add_component('stonehearth:attributes'):set_attribute(attribute_name, value)
   end
end

function entities.add_buff(entity, buff_name)
   if entity and entity:is_valid() then
      return entity:add_component('stonehearth:buffs'):add_buff(buff_name)
   end
end

function entities.remove_buff(entity, buff_name)
   if entity and entity:is_valid() then
      entity:add_component('stonehearth:buffs'):remove_buff(buff_name)
   end
end

function entities.has_buff(entity, buff_name)
   return entity:add_component('stonehearth:buffs'):has_buff(buff_name)
end

function entities.set_posture(entity, posture)
   if entity and entity:is_valid() then
      entity:add_component('stonehearth:posture'):set_posture(posture)
   end
end

function entities.unset_posture(entity, posture)
   if entity and entity:is_valid() then
      local pc = entity:get_component('stonehearth:posture')
      if pc then
         pc:unset_posture(posture)
      end
   end
end

function entities.get_posture(entity)
   return entity:add_component('stonehearth:posture'):get_posture()
end

--[[
   Tell the entity (a mob, probably) to pick up the item
   entity: probably a mob
   item: the thing to pick up
]]
function entities.pickup_item(entity, item)
   assert(item and item:is_valid())
   radiant.check.is_entity(entity)
   radiant.check.is_entity(item)

   local carry_block = entity:get_component('stonehearth:carry_block')
   assert(carry_block)

   local current_item = carry_block:get_carrying()
   if current_item ~= nil and current_item:is_valid() then
      -- cannot pickup an item while carrying another!
      return false
   end

   carry_block:set_carrying(item)

   return true
end

--[[
   Tell the entity (a mob, probably) to drop
   whatever it's carrying at a certain location
   TODO: doesn't the target location have to match the put down animation? Revisit
   entity: probably a mob
   location: the place where we want to put the item
]]
function entities.drop_carrying_on_ground(entity, location)
   radiant.check.is_entity(entity)

   if not location then
      location = radiant.entities.get_location_aligned(entity)
   end
   radiant.check.is_a(location, Point3)

   local item = entities.remove_carrying(entity)
   if item then
      radiant.terrain.place_entity(item, location)
   end
end

function entities.put_carrying_into_entity(entity, target)
   radiant.check.is_entity(entity)

   local item = entities.remove_carrying(entity)
   if item then
      entities.add_child(target, item)
   end
end

--Uses up the thing a person is carrying
function entities.consume_carrying(entity)
   radiant.check.is_entity(entity)

   --if the item has stacks, consume stacks
   --when stacks are zero, remove
   
   local item = entities.get_carrying(entity)
   if item then
      local item_component = item:get_component('item')
      if item_component and item_component:get_stacks() > 1 then
         local stacks = item_component:get_stacks() - 1
         item_component:set_stacks(stacks)
         return item
      end
   end

   local item = entities.remove_carrying(entity)
   if item then
      entities.destroy_entity(item)
   end
   return nil
end

-- consume one "stack" of an entity.  if the entity has an item
-- component and the stacks of that item are > 0, it simply decrements
-- the stack count.  otherwise, it conumes the whole item (i.e. we
-- destroy it!)
-- @param - item to consume
-- @param - number of stacks to consume, if nil, default to 1
-- @returns whether or not the item was consumed
function entities.consume_stack(item, num_stacks)
   local item_component = item:get_component('item')
   local success = false
   local stacks = 0

   if num_stacks == nil then
      num_stacks = 1
   end

   if item_component then
      stacks = item_component:get_stacks()
      if stacks > 0 then
         stacks = math.max(0, stacks - num_stacks)
         item_component:set_stacks(stacks)
         success = true
      end
   end

   if stacks == 0 then
      entities.destroy_entity(item)
   end

   return success
end

-- If the entity is carrying something then increment the number
-- of things the entity is carrying by a number (1 by default). 
-- This is useful when adding one to something the player is already carrying
function entities.increment_carrying(entity, num_to_add)
   local item = entities.get_carrying(entity)
   if not item then
      return false, 'not carrying anything objects'
   end
   local item_component = item:get_component('item')
   if not item_component then 
      return false, 'carried entity has no item component'
   end
   local stacks = item_component:get_stacks()
   if stacks >= item_component:get_max_stacks() then
      return false, 'max stacks on carried object reached'
   end
   if num_to_add == nil then
      num_to_add = 1
   end

   --shouldn't ever be higher than max stacks
   local final_stacks = math.min(item_component:get_max_stacks(), stacks + num_to_add)
   item_component:set_stacks(final_stacks)
   
   return true
end

--- Put an object down on another object
-- @param entity The entity carrying the object
-- @param target The object that will get the new thing added to it
-- @param location (Optional) The location on the target to put the object. 0,0,0 by default
function entities.put_carrying_in_entity(entity, target, location)
   local target_loc = location
   if not target_loc then
      target_loc = Point3(0,0,0)
   end
   local item = entities.remove_carrying(entity)
   if item then
      entities.add_child(target, item, target_loc)
   end
end

--- Helper for the drop functions.
-- Determines the carried item from the entity
-- @param entity The entity that is carrying the droppable item
function entities.remove_carrying(entity)
   local carry_block = entity:get_component('stonehearth:carry_block')
   if carry_block then
      local item = carry_block:get_carrying()
      if item then
         carry_block:set_carrying(nil)
         return item
      end
   end
end

function entities.can_acquire_lease(leased_object, lease_name, lease_holder)
   if not leased_object or not leased_object:is_valid() or
      not lease_holder or not lease_holder:is_valid() then
      return false
   end

   local lease_component = leased_object:get_component('stonehearth:lease')
   if lease_component then
      local can_acquire = lease_component:can_acquire(lease_name, lease_holder)
      return can_acquire
   else
      return true
   end
end

function entities.acquire_lease(leased_object, lease_name, lease_holder, persistent)
   if not leased_object or not leased_object:is_valid() or
      not lease_holder or not lease_holder:is_valid() then
      return false
   end

   persistent = persistent or true

   local lease_component = leased_object:add_component('stonehearth:lease')
   local acquired = lease_component:acquire(lease_name, lease_holder, { persistent = persistent })
   return acquired
end

function entities.release_lease(leased_object, lease_name, lease_holder)
   if not leased_object or not leased_object:is_valid() then
      return true
   end
   
   local lease_component = leased_object:add_component('stonehearth:lease')
   local result = lease_component:release(lease_name, lease_holder)
   return result
end

--[[
   Checks if an entity is next to a location, updated
   to use get_world_grid location.
   TODO: Still relevant with Tony's pathfinder?
   entity: the entity to check
   location: the target location
   returns: true if the entity is adjacent to the specified ocation
]]
function entities.is_adjacent_to(subject, target)
   if radiant.util.is_a(subject, Entity) then
      subject = entities.get_world_grid_location(subject)
      if not subject then
         -- target is not actually in the world at the moment
         return false
      end
   end
   if radiant.util.is_a(target, Entity) then
      local destination = target:get_component('destination')
      if destination then
         return entities.point_in_destination_adjacent(target, subject)
      end
      local rcs = target:get_component('region_collision_shape')
      if rcs and rcs:get_region_collision_type() ~= _radiant.om.RegionCollisionShape.NONE then
         local region = rcs:get_region()
         if region then
            local world_space_region = entities.local_to_world(region:get(), target)
            local adjacent = world_space_region:get_adjacent(false)
            return adjacent:contains(subject)
         end
      end
      target = entities.get_world_grid_location(target)
      if not target then
         -- target is not actually in the world at the moment
         return false
      end
   end
   radiant.check.is_a(subject, Point3)
   radiant.check.is_a(target, Point3)
   return subject:is_adjacent_to(target)
end


function entities.is_adjacent_to_xz(entity, location)
   --local point_a = util:is_a(arg1, Entity) and singleton.get_world_grid_location(arg1) or arg1
   --local point_b = util:is_a(arg2, Entity) and singleton.get_world_grid_location(arg2) or arg2
   local point_a = entities.get_world_grid_location(entity)
   local point_b = location
   radiant.check.is_a(point_a, Point3)
   radiant.check.is_a(point_b, Point3)

   point_a = Point2(a.x, a.z)
   point_b = Point2(b.x, b.z)
   return point_a:is_adjacent_to(point_b)
end

function entities.get_target_table(entity, table_name)
   if not entity or not entity:is_valid() then
      return nil
   end

   local target_tables_component = entity:add_component('stonehearth:target_tables')
   return target_tables_component:get_target_table(table_name)
end

function entities.compare_attribute(entity_a, entity_b, attribute)
   local attributes_a = entity_a:get_component('stonehearth:attributes')
   local attributes_b = entity_b:get_component('stonehearth:attributes')

   if attributes_a and attributes_b then
      local att_a = attributes_a:get_attribute(attribute)
      local att_b = attributes_b:get_attribute(attribute)

      return att_a - att_b
   end

   return 0
end

-- This really shouldn't be here, but until we have a thing for this....
function entities.are_players_hostile(player_a, player_b)
   if entities._is_neutral_player(player_a) then
      return false
   end

   if entities._is_neutral_player(player_b) then
      return false
   end

   return player_a ~= player_b
end

-- we'll use a mapping table later to determine alliances / hostilities
function entities.is_hostile(entity_a, entity_b)
   local player_a = radiant.entities.get_player_id(entity_a)
   if entities._is_neutral_player(player_a) then
      return false
   end

   local player_b = radiant.entities.get_player_id(entity_b)
   if entities._is_neutral_player(player_b) then
      return false
   end

   return player_a ~= player_b
end

-- we'll use a mapping table later to determine alliances / hostilities
function entities.is_friendly(entity_a, entity_b)
   local player_a = radiant.entities.get_player_id(entity_a)
   if entities._is_neutral_player(player_a) then
      return false
   end

   local player_b = radiant.entities.get_player_id(entity_b)
   if entities._is_neutral_player(player_b) then
      return false
   end

   return player_a == player_b
end

-- we'll use a mapping table later to determine alliances / hostilities
function entities._is_neutral_player(player)
   return player == nil or player == '' or player == 'critters'
end

function entities.get_world_speed(entity)
   local speed_attribute = radiant.entities.get_attribute(entity, 'speed')
   speed_attribute = speed_attribute or 100
   local world_speed = speed_attribute / 100
   return world_speed
end

function entities.trace_location(entity, reason)
   return entity:add_component('mob'):trace_transform(reason)
end

function entities.trace_grid_location(entity, reason)
   local last_location = entities.get_world_grid_location(entity)
   local filter_fn = function()
         local current_location = entities.get_world_grid_location(entity)

         if current_location == last_location then
            return false
         else 
            last_location = current_location
            return true
         end
      end

   local trace_impl = entity:add_component('mob'):trace_transform(reason)
   local filtered_trace = FilteredTrace(trace_impl, filter_fn)
   return filtered_trace
end

function entities._point_in_destination(which, entity, pt)
   local destination = entity:get_component('destination')
   if destination then
      local region = destination['get_' .. which](destination)
      if region then
         local region3 = region:get()
         if region3 then
            local rotated = entities.local_to_world(region3, entity)
            return rotated:contains(pt)
         end
     end
   end
end

function entities.point_in_destination_region(entity, pt)
   return entities._point_in_destination('region', entity, pt)
end

function entities.point_in_destination_reserved(entity, pt)
   return entities._point_in_destination('reserved', entity, pt)
end

function entities.point_in_destination_adjacent(entity, pt)
   return entities._point_in_destination('adjacent', entity, pt)
end

function entities.is_material(entity, materials)
   if not entity or not entity:is_valid() then
      return false
   end
   
   radiant.check.is_entity(entity)
   local is_material = false
   local material_component = entity:get_component('stonehearth:material')
   if material_component then
      is_material = material_component:is(materials)
   end
   return is_material
end

function entities.local_to_world(pt, e)
   return _radiant.physics.local_to_world(pt, e)
end

function entities.world_to_local(pt, e)
   return _radiant.physics.world_to_local(pt, e)
end

-- extract the full sized entity from an iconic entity proxy.  `component_name`
-- is optional.  if specified, the component for the full sized item matching that
-- name will be returned as the 2nd parameter.  if the entity is not a placable
-- item, the function returns nil
--
--    @param entity : the proxy entity
--    @param component_name : (optional) the name of the component on the full
--           sized entity you also want returned
--
function entities.unwrap_iconic_item(entity, component_name)
   local root_entity, root_component
   local proxy = entity:get_component('stonehearth:iconic_form')
   if proxy then
      root_entity = proxy:get_root_entity()
      if component_name then
         root_component = root_entity:get_component(component_name)
      end
   end
   return root_entity, root_component
end

--Return whether the entitiy is frightened of the target
--Compares the entity's courage score to the target's menace score
--returns false if either is missing the appropriate component
function entities.is_frightened_of(entity, target)
   local entity_attribute_component = entity:get_component('stonehearth:attributes')
   local target_attribute_component = target:get_component('stonehearth:attributes')
   local entity_courage = entity_attribute_component and entity_attribute_component:get_attribute('courage') or 0
   local target_menace = target_attribute_component and target_attribute_component:get_attribute('menace') or 0
   return entity_courage < target_menace
end

-- assumes the hiearchy will NOT be modified during iteration
function entities.for_all_children(entity, cb)
   local ec = entity:get_component('entity_container')  
   if ec then
      for _, child in ec:each_child() do
         entities.for_all_children(child, cb)
      end
   end
   cb(entity)
end

entities.__init()
return entities
