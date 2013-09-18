local entities = {}
local singleton = {}

local Point3 = _radiant.csg.Point3

function entities.__init()
   singleton._entity_dtors = {}
end

--[[
Opposite of _init_entity. Given a uri, remove
component influences from the entity.
--]]
-- xxx: this function must go, too -- tony
function entities.xxx_unregister_from_entity(entity, mod_name, entity_name)
   assert(entity_name)
   local uri = _radiant.sim.xxx_get_entity_uri(mod_name, entity_name)
   local obj = radiant.resources.load_json(uri)

   if obj then
      if obj.components then
         for name, json in pairs(obj.components) do
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

function entities.create_entity(arg1, arg2)
   if not arg2 then
      local entity_ref = arg1 -- something like 'entity(stonehearth, wooden_sword)'
      assert(entity_ref:sub(1, 7) == 'entity(')
      --radiant.log.info('creating entity %s', entity_ref)
      return native:create_entity_by_ref(entity_ref)
   end
   local mod_name = arg1 -- 'stonehearth'
   local entity_name = arg2 -- 'wooden_sword'
   --radiant.log.info('creating entity %s, %s', mod_name, entity_name)
   return native:create_entity(mod_name, entity_name)
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

function entities.xxx_inject_into_entity(entity, mod_name, entity_name)
   if not entity_name then
      assert(false)
   end
   return _radiant.sim.xxx_extend_entity(entity, mod_name, entity_name)
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

entities.__init()
return entities
