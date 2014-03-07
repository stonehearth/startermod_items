local BuildService = class()

function BuildService:__init(datastore)
end


--- Convert the proxy to an honest to goodness entity
-- The entity_map contains a map from proxy entity ids to the entities that
-- get created for those proxies.
function BuildService:_unpackage_proxy_data(proxy, entity_map)

   -- Create the entity for this proxy and update the entity_map
   local entity = radiant.entities.create_entity(proxy.entity_uri)
   entity_map[proxy.entity_id] = entity
   
   -- Initialize all the simple components.
   radiant.entities.set_faction(entity, self._faction)
   if proxy.components then
      for name, data in pairs(proxy.components) do
         entity:add_component(name, data)
      end
   end

   -- Initialize the construction_data and entity_container components.  We can't
   -- handle these with :load_from_json(), since the actual value of the entities depends on
   -- what gets created server-side.  So instead, look up the actual entity that
   -- got created in the entity map and shove that into the component.
   if proxy.dependencies then
      local cd = entity:add_component('stonehearth:construction_data')
      for _, dependency_id in ipairs(proxy.dependencies) do
         local dep = entity_map[dependency_id];
         assert(dep, string.format('could not find dependency entity %d in entity map', dependency_id))
         cd:add_dependency(dep);
      end
   end
   if proxy.children then
      for _, child_id in ipairs(proxy.children) do
         local child_entity = entity_map[child_id]
         assert(child_entity, string.format('could not find child entity %d in entity map', child_id))
         entity:add_component('entity_container'):add_child(child_entity)
      end
   end
   return entity
end

function BuildService:build_structures(faction, changes)
   local root = radiant.entities.get_root_entity()
   local city_plan = root:add_component('stonehearth:city_plan')
   
   self._faction = faction

   -- Create a new entity for each entry in the changes list.  The client is
   -- responsible for creating the list in such a way that we can do this
   -- naievely (e.g. when we encounter an item in some proxy's children list, that
   -- child is guarenteed to have already been processed)
   local entity_map = {}
   for _, proxy in ipairs(changes) do
      local entity = self:_unpackage_proxy_data(proxy, entity_map)

      -- If this proxy needs to be added to the terrain, go ahead and do that now.
      -- This will also create fabricators for all the descendants of this entity
      if (proxy.add_to_build_plan) then
         city_plan:add_blueprint(entity)
      end
   end
   
   return { success = true }
end

return BuildService

