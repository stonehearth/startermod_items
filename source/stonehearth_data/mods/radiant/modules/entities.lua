local entities = {}
local singleton = {}

local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3

function entities.__init()
   singleton._entity_dtors = {}
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
   radiant.check.is_entity(entity)
   local id = entity:get_id()
   local dtors = singleton._entity_dtors[id]
   if dtors then
      for _, dtor in ipairs(dtors) do
         dtor()
      end
      singleton._entity_dtors[id] = nil
   end
   _radiant.sim.destroy_entity(entity)
end

function entities.add_child(parent, child, location)
   radiant.check.is_entity(parent)
   radiant.check.is_entity(child)

   local component = parent:add_component('entity_container')

   component:add_child(child)
   if location then
      entities.move_to(child, location)
   end
end

function entities.remove_child(parent, child)
   radiant.check.is_entity(parent)
   radiant.check.is_entity(child)

   local component = parent:get_component('entity_container')

   component:remove_child(child:get_id())
   entities.move_to(child, Point3(0, 0, 0))
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

function entities.get_world_grid_location(entity)
   local mob = entity:get_component('mob')
   return mob and mob:get_world_grid_location() or Point3(0, 0, 0)
end

function entities.distance_between(entity_a, entity_b)
   local loc_a = radiant.entities.get_world_grid_location(entity_a)
   local loc_b = radiant.entities.get_world_grid_location(entity_b)

   return loc_a:distance_to(loc_b)
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

function entities.get_entity_data(entity, key)
   if entity then
      --xxx: what is this : business? (Tony asked me to put this comment here)
      --I hear this issue is solved on his machine
      local uri = entity:get_uri()
      if uri and #uri > 0 and uri ~= ':'then
         local json = radiant.resources.load_json(uri)
         if json.entity_data then
            return json.entity_data[key]
         end
      end
   end
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
   local carry_block = entity:add_component('carry_block')
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

-- id here can be an int (e.g. 999) or uri (e.g. '/o/stores/server/objects/999')
function entities.get_entity(id)
   return _radiant.sim.get_entity(id)
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
   return entity:add_component('stonehearth:attributes'):get_attribute(attribute_name)
end


function entities.set_attribute(entity, attribute_name, value)
   entity:add_component('stonehearth:attributes'):set_attribute(attribute_name, value)
end

function entities.add_buff(entity, buff_name)
   entity:add_component('stonehearth:buffs'):add_buff(buff_name)
end

function entities.remove_buff(entity, buff_name)
   entity:add_component('stonehearth:buffs'):remove_buff(buff_name)
end

function entities.has_buff(entity, buff_name)
   return entity:add_component('stonehearth:buffs'):has_buff(buff_name)
end

function entities.set_posture(entity, posture)
   entity:add_component('stonehearth:posture'):set_posture(posture)
end

function entities.unset_posture(entity, posture)
   entity:add_component('stonehearth:posture'):unset_posture(posture)
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
   radiant.check.is_entity(entity)
   radiant.check.is_entity(item)

   local carry_block = entity:get_component('carry_block')
   if carry_block then
      if item then
         if carry_block:is_carrying() then
            -- cannot pickup an item while carrying another!
            return false
         end
         local parent = item:add_component('mob'):get_parent()
         if parent then
            entities.remove_child(parent, item)
         end
         radiant.entities.set_posture(entity, 'carrying')
         radiant.entities.add_buff(entity, 'stonehearth:buffs:carrying')
         carry_block:set_carrying(item)
         entities.move_to(item, Point3(0, 0, 0))
      else
         radiant.entities.unset_posture(entity, 'carrying')
         radiant.entities.remove_buff(entity, 'stonehearth:buffs:carrying')
         carry_block:clear_carrying()
      end
   end
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

   local item = entities._drop_helper(entity)
   if item then
      radiant.terrain.place_entity(item, location)
   end
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
   local item = entities._drop_helper(entity)
   if item then
      entities.add_child(target, item, target_loc)
   end
end

--- Helper for the drop functions.
-- Determines the carried item from the entity
-- @param entity The entity that is carrying the droppable item
function entities._drop_helper(entity)
   local carry_block = entity:get_component('carry_block')
   if carry_block then
      local item = carry_block:get_carrying()
      if item then
         radiant.entities.unset_posture(entity, 'carrying')
         radiant.entities.remove_buff(entity, 'stonehearth:buffs:carrying')
         carry_block:clear_carrying()
         return item
      end
   end
end


--[[
   Checks if an entity is next to a location, updated
   to use get_world_grid location.
   TODO: Still relevant with Tony's pathfinder?
   entity: the entity to check
   location: the target location
   returns: true if the entity is adjacent to the specified ocation
]]
function entities.is_adjacent_to(entity, location)
   -- xxx: this style doesn't work until we fix util:is_a().
   --local point_a = util:is_a(arg1, Entity) and singleton.get_world_grid_location(arg1) or arg1
   --local point_b = util:is_a(arg2, Entity) and singleton.get_world_grid_location(arg2) or arg2
   local point_a = entities.get_world_grid_location(entity)
   local point_b = location

   radiant.check.is_a(point_a, Point3)
   radiant.check.is_a(point_b, Point3)
   return point_a:is_adjacent_to(point_b)
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

function entities.get_target_table_top(entity, table_name)
   local target_tables = entity:get_component('target_tables')
   local top_entry = target_tables:get_top(table_name)

   if top_entry then
      return top_entry.target
   end

   return nil
end

function entities.kill_entity(entity)
   --radiant.entities.destroy_entity(entity)
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

function entities.is_hostile(entity_a, entity_b)
   -- xxx: this check shouldn't be in the generic "is_hostile" function.  what
   -- happens when we add things that aren't made of meat? (e.g. robots?)
   local material = entity_b:get_component('stonehearth:material')
   if not material or not material:is('meat') then
      return false
   end

   local faction_a = entity_a:add_component('unit_info'):get_faction()
   local faction_b = entity_b:add_component('unit_info'):get_faction()

   return faction_a and faction_b and
          faction_a ~= '' and faction_b ~= '' and
          faction_a ~= faction_b
end

function entities.on_entity_moved(entity, fn, reason)
   reason = reason and reason or 'on_entity_moved promise'
   return entity:add_component('mob'):trace_transform(reason):on_changed(fn)
end

entities.__init()
return entities
