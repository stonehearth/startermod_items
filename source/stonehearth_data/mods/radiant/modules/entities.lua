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
   if not ref then
      return _radiant.sim.create_empty_entity()
   end
   radiant.log.info('creating entity %s', ref)
   return _radiant.sim.create_entity_by_ref(ref)
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
   entities.move_to(child, location)
end

function entities.remove_child(parent, child)
   radiant.check.is_entity(parent)
   radiant.check.is_entity(child)

   local component = parent:get_component('entity_container')

   component:remove_child(child)
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

   local children = component:get_children()

   local found = false
   for id, child in children:items() do
      if child_id == id then
        found = true
      end
   end

   return found
end

function entities.get_faction(entity)
   local unit_info = entity:get_component('unit_info')
   return unit_info and unit_info:get_faction() or nil
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

-- id here can be an int (e.g. 999) or uri (e.g. '/o/stores/server/objects/999')
function entities.get_entity(id)
   return _radiant.sim.get_entity(id)
end


function entities.get_animation_table_name(entity)
   local name
   local render_info = entity:get_component('render_info')
   if render_info then
      name = render_info:get_animation_table_name()
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
         carry_block:set_carrying(item)
         entities.move_to(item, Point3(0, 0, 0))
      else
         carry_block:set_carrying(nil)
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
function entities.drop_carrying(entity, location)
   radiant.check.is_entity(entity)
   radiant.check.is_a(location, Point3)

   local carry_block = entity:get_component('carry_block')
   if carry_block then
      local item = carry_block:get_carrying()
      if item then
         carry_block:set_carrying(nil)
         radiant.terrain.place_entity(item, location)
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

entities.__init()
return entities
