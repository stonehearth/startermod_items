local Builder = class()

function Builder:initialize()
end

function Builder:is_blueprint(entity)
   return entity and entity:is_valid() and entity:get_component('stonehearth:construction_progress')
end

function Builder:get_construction_data_for(entity)
   -- if we're blueprint, just grab the construction progress component right now
   local cp = entity:get_component('stonehearth:construction_data')
   if cp then
      return cp:get_data()
   end
   -- are we a fabricator?  if so, grab the cp for the blueprint
   local fab = entity:get_component('stonehearth:fabricator')
   if fab then
      return self:get_progress_component_for(fab:get_data().blueprint)
   end
   -- no luck. =(
   return nil
end

function Builder:get_construction_type(entity)
   local blueprint = self:get_blueprint_for(entity)
end

function Builder:merge_buildings(buildings)
   local i = 2
   local merge_into = buildings[1]  
   
   local function merge_next()
      if buildings[i] then
         _radiant.call('stonehearth:merge_buildings', merge_into, buildings[i])
            :done(function(e)
                  i = i + 1
                  merge_next()
                  local id, blueprint = next(e.blueprints)
                  if blueprint and blueprint:is_valid() then
                     stonehearth.selection:select_entity(blueprint)
                  end
               end)
      end
   end
end

function Builder:create_building(building)
   local changed = {}

   self:_compute_package_order({}, changed, building)
   for i, proxy in ipairs(changed) do
      changed[i] = self:_package_proxy(proxy)
   end

   _radiant.call('stonehearth:build_structures', changed)
      :done(function(e)
            local id, blueprint = next(e.blueprints)
            if blueprint and blueprint:is_valid() then
               stonehearth.selection:select_entity(blueprint)
            end
         end)
end

--- Add all descendants of proxy to the 'order' list in package order.
-- The client is responsible for providing a list of proxies to the
-- server in an order that's trivial to consume.  This means proxies
-- must be defined before they can be referenced.  No problem!  Just do
-- a post-order traversal of the proxy tree.  Since we're concerned about
-- both the 'children' and 'dependency' trees, this is actually a weird
-- directed graph (which means we need a visited map to keep track of where
-- we've been).
function Builder:_compute_package_order(visited, order, proxy)
   local id = proxy:get_id()

   if not visited[id] then
      visited[id] = true

      for id, child in pairs(proxy:get_children()) do
         self:_compute_package_order(visited, order, child)
      end
      for id, dependency in pairs(proxy:get_dependencies()) do
         self:_compute_package_order(visited, order, dependency)
      end
      table.insert(order, proxy)
   end
end


--- Package a single proxy for deliver to the server
function Builder:_package_proxy(proxy)
   local package = {}

   local entity =  proxy:get_entity()  
   package.entity_uri = entity:get_uri()
   package.entity_id = entity:get_id()
   
   local mob = entity:get_component('mob')   
   if mob then
      local parent = mob:get_parent()
      if not parent then
         package.add_to_build_plan = true
      end
   end
   
   -- Package up the trival components...
   package.components = {}
   for _, name in ipairs({'mob', 'destination'}) do
      local component = entity:get_component(name)
      package.components[name] = component and component:serialize() or {}
   end   
   
   -- Package up the stonehearth:construction_data component.  This is almost
   -- trivial, but not quite.
   local datastore = entity:get_component('stonehearth:construction_data')
   if datastore then
      -- remove the paint_mode.  we just set it here so the blueprint
      -- would be rendered differently
      local data = datastore:get_data()
      data.paint_mode = nil
      package.components['stonehearth:construction_data'] = data
   end

   local building = proxy:get_building()
   if building then
      package.building_id = building:get_id()
   end
   
   -- Package the child and dependency lists.  Build it as a table first to
   -- make sure we don't get duplicates in the children/dependency list,
   -- then shove it into the package.dependencies
   local all_dependencies = {}
   local children = proxy:get_children()
   if next(children) then
      package.children = {}
      for id, child in pairs(children) do
         -- TODO(tony): fix this!  Hack to work around adding portals as dependencies of proxies.
         if child:get_entity():get_component('stonehearth:portal') == nil then
            all_dependencies[id] = true
         end
         table.insert(package.children, id)
      end
   end
   
   local dependencies = proxy:get_dependencies()
   if next(dependencies) then
      for id, _ in pairs(dependencies) do
         all_dependencies[id] = true
      end
   end

   if next(all_dependencies) then
      package.dependencies = {}
      for id, _ in pairs(all_dependencies) do
         table.insert(package.dependencies, id)
      end
   end

   -- borrowed scaffolding...
   local borrowing_scaffolding = proxy:get_loaning_scaffolding_to()
   if next(borrowing_scaffolding) then
      package.loan_scaffolding_to = {}
      for id, _ in pairs(borrowing_scaffolding) do
         table.insert(package.loan_scaffolding_to, id)
      end
   end

   return package
end

return Builder