local BuildService = class()

function BuildService:__init(datastore)
end

function BuildService:initialize()
end

function BuildService:restore(saved_variables)
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
   radiant.entities.set_player_id(entity, self._player_id)
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
      local progress = entity:add_component('stonehearth:construction_progress')
      for _, dependency_id in ipairs(proxy.dependencies) do
         local dep = entity_map[dependency_id];
         assert(dep, string.format('could not find dependency entity %d in entity map', dependency_id))
         progress:add_dependency(dep);
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

function BuildService:build_structures(session, changes)
   local root = radiant.entities.get_root_entity()
   local town = stonehearth.town:get_town(session.player_id)
   
   self._faction = session.faction
   self._player_id = session.player_id

   -- Create a new entity for each entry in the changes list.  The client is
   -- responsible for creating the list in such a way that we can do this
   -- naievely (e.g. when we encounter an item in some proxy's children list, that
   -- child is guarenteed to have already been processed)
   local entity_map = {}
   for _, proxy in ipairs(changes) do
      local entity = self:_unpackage_proxy_data(proxy, entity_map)
      
      -- If this proxy needs to be added to the terrain, go ahead and do that now.
      -- This will also create fabricators for all the descendants of this entity
      if proxy.add_to_build_plan then
         self:_add_blueprint(entity)
         town:add_construction_project(entity)
      end
   end
   
   return { success = true }
end

function BuildService:_add_blueprint(blueprint, location)
   -- create a new fabricator...   to... you know... fabricate
   local project = self:_create_fabricator(blueprint)
   radiant.terrain.place_entity(project, location)
end

function BuildService:_create_fabricator(blueprint)
   -- either you're a fabricator or you contain things which may be fabricators.  not
   -- both!
   local fabricator = radiant.entities.create_entity()
   self:_init_fabricator(fabricator, blueprint)
   self:_init_fabricator_children(fabricator, blueprint)
   return fabricator
end

function BuildService:_init_fabricator(fabricator, blueprint)
   local blueprint_mob = blueprint:add_component('mob')
   local parent = blueprint_mob:get_parent()
   if parent then
      parent:add_component('entity_container'):add_child(fabricator)
   end
   local transform = blueprint_mob:get_transform()
   fabricator:add_component('mob'):set_transform(transform)
   
   fabricator:set_debug_text('fabricator for ' .. blueprint:get_debug_text())
   
   if blueprint:get_component('stonehearth:construction_data') then
      local name = tostring(blueprint)
      fabricator:add_component('stonehearth:fabricator')
                     :start_project(name, blueprint)
      fabricator:add_component('render_info')
                     :set_material('materials/blueprint_gridlines.xml')
                     --:set_model_mode('opaque')
   end
end

function BuildService:_init_fabricator_children(fabricator, blueprint)
   local ec = blueprint:get_component('entity_container')  
   if ec then
      for id, child in ec:each_child() do
         local fc = self:_create_fabricator(child)
         fabricator:add_component('entity_container'):add_child(fc)
      end
   end
end

return BuildService
