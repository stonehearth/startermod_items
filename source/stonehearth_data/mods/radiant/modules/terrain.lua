local Point3 = _radiant.csg.Point3

local Terrain = {}
local singleton = {}

local _terrain = radiant._root_entity:add_component('terrain')

function Terrain.place_entity(entity, location)
   radiant.entities.add_child(radiant._root_entity, entity, location)
   local render_info = entity:add_component('render_info')
   local variant = render_info:get_model_variant()
   if variant == '' then
      render_info:set_model_variant('iconic')
   end

   if type(location) == "table" then
      location = Point3(location.x, location.y, location.z)
   end
   _terrain:place_entity(entity, location)

   local id = entity:get_id()
   if singleton._entity_placefns and singleton._entity_placefns[id] then
       for i, fn in ipairs(singleton._entity_placefns[id]) do
         fn()
      end
   end

end

--- Store a function to call when we successfully place something
function Terrain.on_place(entity, placefn)
   radiant.check.is_entity(entity)
   local id = entity:get_id()
   if not singleton._entity_placefns then
      singleton._entity_placefns = {}
   end
   if not singleton._entity_placefns[id] then
      singleton._entity_placefns[id] = {}
   end
   table.insert(singleton._entity_placefns[id], placefn)
end

function Terrain.remove_entity(entity)
   radiant.entities.remove_child(radiant._root_entity, entity)
   local id = entity:get_id()
   if singleton._entity_removefns and singleton._entity_removefns[id] then
       for i, fn in ipairs(singleton._entity_removefns[id]) do
         fn()
      end
   end
end

--- Store a function to call when removing an item
-- REVIEW COMMENT, REMOVE BEFORE PULLING TO DEVELOP:
-- I can't get remove_entity and place_entity to work
-- correctly, so I'm not actually using this on_remove right now. Would you like
-- me to remove it for now? I know it works.
function Terrain.on_remove(entity, removefn)
   radiant.check.is_entity(entity)
   local id = entity:get_id()
   if not singleton._entity_removefns then
      singleton._entity_removefns = {}
   end
   if not singleton._entity_removefns[id] then
      singleton._entity_removefns[id] = {}
   end
   table.insert(singleton._entity_removefns[id], removefn)
end

function Terrain.trace_world_entities(reason, added_cb, removed_cb)
   local ec = radiant.entities.get_root_entity():add_component('entity_container');
   local children = ec:get_children()

   -- put a trace on the root entity container to detect when items
   -- go on and off the terrain.  each item is forwarded to the
   -- appropriate tracker.
   local trace = children:trace('radiant.terrain: ' .. reason)
                     :on_added(added_cb)
                     :on_removed(removed_cb)
   for id, entity in children:items() do
      added_cb(id, entity)
   end
   return trace
end

function Terrain.get_world_entities()
   local ec = radiant.entities.get_root_entity():add_component('entity_container');
   local children = ec:get_children()
   return children:items()
end

return Terrain
