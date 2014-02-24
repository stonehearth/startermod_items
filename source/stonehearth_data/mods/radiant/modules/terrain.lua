local Point3 = _radiant.csg.Point3

local Terrain = {}
local singleton = {}

local _terrain = radiant._root_entity:add_component('terrain')

function Terrain.place_entity(entity, location)
   local render_info = entity:add_component('render_info')
   local variant = render_info:get_model_variant()
   if variant == '' then
      render_info:set_model_variant('iconic')
   end

   radiant.entities.add_child(radiant._root_entity, entity, location)
   if location then
      if type(location) == "table" then
         location = Point3(location.x, location.y, location.z)
      end
      _terrain:place_entity(entity, location)
   end
end

function Terrain.remove_entity(entity)
   radiant.entities.remove_child(radiant._root_entity, entity)
end

function Terrain.trace_world_entities(reason, added_cb, removed_cb)
   local root = radiant.entities.get_root_entity()
   local ec = radiant.entities.get_root_entity():add_component('entity_container');

   -- put a trace on the root entity container to detect when items
   -- go on and off the terrain.  each item is forwarded to the
   -- appropriate tracker.
   return ec:trace_children('radiant.terrain: ' .. reason)
                        :on_added(function (id, entity) 
                              if entity and entity:is_valid() then
                                 added_cb(id, entity)
                              end
                           end)
                        :on_removed(removed_cb)
                        :push_object_state()
end

function Terrain.get_entities_in_box(box)
   local results = {}

   for id, entity in radiant.terrain.each_world_entity() do
      if entity then
         local mob_component = entity:get_component('mob')
         if mob_component then
            local location = mob_component:get_world_grid_location()
            if box:contains(location) then
               table.insert(results, entity)
            end
         end
      end
   end   

   return ipairs(results)
end


function Terrain.each_world_entity()
   local ec = radiant.entities.get_root_entity():add_component('entity_container');
   return ec:each_child()
end

function Terrain.get_height(location)
   return _terrain:get_height(location)
end

return Terrain
