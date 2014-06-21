local entities = {}
--local singleton = {}

local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local Entity = _radiant.om.Entity
local log = radiant.log.create_logger('entities')
local rng = _radiant.csg.get_default_rng()

function entities.__init()
end

function entities.get_root_entity()
   return radiant._root_entity
end

function entities.create_entity(ref)
   if not ref or #ref == 0 then
      return _radiant.sim.create_empty_entity()
   end
   return _radiant.sim.create_entity(ref)
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
         for id, child in ec:each_child() do
            entities.destroy_entity(child)
         end
      end
      _radiant.sim.destroy_entity(entity)
   end
end

function entities.create_proxy_entity(use_default_adjacent_region)
   if use_default_adjacent_region == nil then
      use_default_adjacent_region = false
   end

   local proxy_entity = radiant.entities.create_entity()
   proxy_entity:set_debug_text('proxy entity')

   if not use_default_adjacent_region then
      -- cache the origin region since we use this a lot
      if entities.origin_region == nil then
         entities.origin_region = _radiant.sim.alloc_region()
         entities.origin_region:modify(
            function(region3)
               region3:add_unique_cube(Cube3(Point3(0, 0, 0), Point3(1, 1, 1)))
            end
         )
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
   return mob and mob:get_parent() or nil
end

function entities.add_child(parent, child, location)
   radiant.check.is_entity(parent)
   radiant.check.is_entity(child)

   local component = parent:add_component('entity_container')

   component:add_child(child)
   if location then
      entities.move_to(child, location)
   else
   end
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

function entities.get_faction(entity)
   local unit_info = entity:get_component('unit_info')
   return unit_info and unit_info:get_faction() or nil
end

function entities.get_player_id(entity)
   local unit_info = entity:get_component('unit_info')
   return unit_info and unit_info:get_player_id() or nil
end

function entities.is_owned_by_player(entity, player_id)
   return entities.get_player_id(entity) == player_id
end

function entities.get_kingdom(entity)
   local unit_info = entity:get_component('unit_info')
   return unit_info and unit_info:get_kingdom() or nil
end

function entities.is_entity(entity)
   if type(entity.get_type_name) == 'function' then
      return entity:get_type_name() == 'class radiant::om::Entity'
   end
   return false
end

function entities.set_faction(entity, faction)
   assert(entity, 'no entity passed to set_faction')
   assert(faction, 'no faction passed to set_faction')
   if entities.is_entity(faction) then
      faction = faction:add_component('unit_info'):get_faction()
   end
   return entity:add_component('unit_info'):set_faction(faction)
end

function entities.set_player_id(entity, player_id)
   assert(entity, 'no entity passed to set_player_id')
   assert(player_id, 'no player_id passed to set_player_id')
   if entities.is_entity(player_id) then
      player_id = player_id:add_component('unit_info'):get_player_id()
   end
   return entity:add_component('unit_info'):set_player_id(player_id)
end

function entities.get_world_location(entity)
   local mob = entity:get_component('mob')
   if not mob then
      error(tostring(entity) .. ' has no mob component')
   end
   return mob:get_world_location()
end

function entities.get_world_grid_location(entity)
   local mob = entity:get_component('mob')
   if not mob then
      error(tostring(entity) .. ' has no mob component')
   end
   return mob:get_world_grid_location()
end

--- Given an object, find a place near it
--  Make sure it doesn't come in exactly on top of the previous object
-- TODO: this could pick something down a cliff or across a long fence...
function entities.pick_nearby_location(entity, radius)
   local target_location = entities.get_world_grid_location(entity)
   local dx = rng:get_int(-radius, radius)
   local dz = rng:get_int(-radius, radius)
   if dx == 0 then
      dx = dx + 1
   end
   if dz == 0 then
      dz = dz + 1
   end
   local destination = Point3(target_location)
   destination.x = destination.x + dx
   destination.z = destination.z + dz
   return destination
end

function entities.grid_distance_between(object_a, object_b)
   local mob

   if radiant.util.is_a(object_a, Entity) then
      mob = object_a:get_component('mob')
      assert(mob)
      object_a = mob:get_world_grid_location()
   end
   if radiant.util.is_a(object_b, Entity) then
      mob = object_b:get_component('mob')
      assert(mob)
      object_b = mob:get_world_grid_location()
   end
   -- xxx: verify a and b are both Point3s...
   return object_a:distance_to(object_b)
end

function entities.distance_between(object_a, object_b)
   local mob

   if radiant.util.is_a(object_a, Entity) then
      mob = object_a:get_component('mob')
      assert(mob)
      object_a = mob:get_world_location()
   end
   if radiant.util.is_a(object_b, Entity) then
      mob = object_b:get_component('mob')
      assert(mob)
      object_b = mob:get_world_location()
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

function entities.turn_to_face(entity, arg2)
   --local location = util:is_a(arg2, Entity) and singleton.get_world_grid_location(arg2) or arg2
   local location
   if radiant.util.is_a(arg2, Point3) then
      location = arg2
   elseif radiant.util.is_a(arg2, Entity) then
      location = entities.get_world_grid_location(arg2)
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
   entity:add_component('stonehearth:thought_bubble'):unset_thought(uri)
end

function entities.get_location_aligned(entity)
   radiant.check.is_entity(entity)
   if entity then
      return entity:add_component('mob'):get_grid_location()
   end
end

function entities.get_world_grid_location(entity)
   radiant.check.is_entity(entity)
   if entity and entity:add_component('mob') then
      return entity:add_component('mob'):get_world_grid_location()
   end
end

function entities.get_equipped_item(entity, slot)
   local equipment_component = entity:get_component('stonehearth:equipment')
   if equipment_component == nil then
      return nil
   end

   local item = equipment_component:get_item_in_slot(slot)
   return item
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
   --radiant.check.is_a(component, UnitInfo)

   return component:get_display_name()
end

function entities.set_display_name(entity, name)
   radiant.check.is_entity(entity)
   radiant.check.is_string(name)

   local component = entity:add_component('unit_info')
   --radiant.check.is_a(component, UnitInfo)

   component:set_display_name(name)
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
   entities.move_to(item, Point3.zero)

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

function entities.increment_carrying(entity)
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
   item_component:set_stacks(stacks + 1)
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
   end
   if radiant.util.is_a(target, Entity) then
      local destination = target:get_component('destination')
      if destination then
         return entities.point_in_destination_adjacent(target, subject)
      end
      target = entities.get_world_grid_location(target)
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
      local ferocity_a = attributes_a:get_attribute(attribute)
      local ferocity_b = attributes_b:get_attribute(attribute)

      return ferocity_a - ferocity_b
   end

   return 0
end

-- we'll use a mapping table later to determine alliances / hostilities
function entities.is_hostile(entity_a, entity_b)
   local faction_a = radiant.entities.get_faction(entity_a)
   local faction_b = radiant.entities.get_faction(entity_b)

   if faction_a == 'critter' or faction_b == 'critter' then
      return false
   end

   return faction_a and faction_b and
          faction_a ~= '' and faction_b ~= '' and
          faction_a ~= faction_b
end

function entities.on_entity_moved(entity, fn, reason)
   reason = reason and reason or 'on_entity_moved promise'
   return entity:add_component('mob'):trace_transform(reason):on_changed(fn)
end

function entities.get_world_speed(entity)
   local speed_attribute = radiant.entities.get_attribute(entity, 'speed')
   speed_attribute = speed_attribute or 50

   local world_speed = math.floor(50 + (50 * speed_attribute / 60)) / 100
   -- fudge factor for tuning speed globally up or down
   world_speed = world_speed * 1.2
   return world_speed
end

-- xxx: prefer this over on_entity_moved()
function entities.trace_location(entity, reason)
   return entity:add_component('mob'):trace_transform(reason)
end

function entities._point_in_destination(which, entity, pt)
   local destination = entity:get_component('destination')
   if destination then
      local region = destination['get_' .. which](destination)
      if region then
         local region3 = region:get()
         if region3 then      
            local mob = entity:get_component('mob')
            if mob then
               pt = pt - mob:get_world_grid_location()      
            end
            return region3:contains(pt)
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

function entities.same_entity(e0, e1)
   return e0 and e1 and e0:is_valid() and e1:is_valid() and e0:equals(e1)
end

function entities.local_to_world(pt, e)
   return _radiant.physics.local_to_world(pt, e)
end

function entities.world_to_local(pt, e)
   return _radiant.physics.world_to_local(pt, e)
end

entities.__init()
return entities
