local BuildService = class()

function BuildService:__init(datastore)
end

function BuildService:initialize()
   self._sv = self.__saved_variables:get_data()
   if not self._sv.next_building_id then
      self._sv.next_building_id = 1 -- used to number newly created buildings
      self.__saved_variables:mark_changed()
   end
end

function BuildService:restore(saved_variables)
end


--- Convert the proxy to an honest to goodness blueprint entity
-- The blueprint_map contains a map from proxy entity ids to the entities that
-- get created for those proxies.
function BuildService:_unpackage_proxy_data(proxy, blueprint_map)

   -- Create the blueprint for this proxy and update the blueprint_map
   local blueprint = radiant.entities.create_entity(proxy.entity_uri)
   blueprint_map[proxy.entity_id] = blueprint
   
   -- Initialize all the simple components.
   radiant.entities.set_faction(blueprint, self._faction)
   radiant.entities.set_player_id(blueprint, self._player_id)
   if proxy.components then
      for name, data in pairs(proxy.components) do
         blueprint:add_component(name, data)
      end
   end

   -- all blueprints have a 'stonehearth:construction_progress' component to track whether
   -- or not they're finished.  dependencies are also tracked here.
   local progress = blueprint:add_component('stonehearth:construction_progress')
  
   -- Initialize the construction_data and entity_container components.  We can't
   -- handle these with :load_from_json(), since the actual value of the entities depends on
   -- what gets created server-side.  So instead, look up the actual entity that
   -- got created in the entity map and shove that into the component.
   if proxy.dependencies then      
      for _, dependency_id in ipairs(proxy.dependencies) do
         local dep = blueprint_map[dependency_id];
         assert(dep, string.format('could not find dependency blueprint %d in blueprint map', dependency_id))
         progress:add_dependency(dep);
      end
   end
   if proxy.children then
      local ec = blueprint:add_component('entity_container')
      for _, child_id in ipairs(proxy.children) do
         local child_entity = blueprint_map[child_id]
         assert(child_entity, string.format('could not find child entity %d in blueprint map', child_id))
         ec:add_child(child_entity)
      end
   end

   return blueprint
end

function BuildService:build_structures(session, proxies)
   local root = radiant.entities.get_root_entity()
   local town = stonehearth.town:get_town(session.player_id)
   local result = {
      blueprints = {}
   }

   self._faction = session.faction
   self._player_id = session.player_id

   -- Create a new entity for each entry in the proxies list.  The client is
   -- responsible for creating the list in such a way that we can do this
   -- naievely (e.g. when we encounter an item in some proxy's children list, that
   -- child is guarenteed to have already been processed)
   local entity_map = {}
   for _, proxy in ipairs(proxies) do
      local blueprint = self:_unpackage_proxy_data(proxy, entity_map)
      
      -- If this proxy needs to be added to the terrain, go ahead and do that now.
      -- This will also create fabricators for all the descendants of this entity
      if proxy.add_to_build_plan then
         blueprint:add_component('unit_info')
                  :set_display_name(string.format('!! Building %d', radiant.get_realtime()))
         self:_begin_construction(blueprint)
         town:add_construction_blueprint(blueprint)

         result.blueprints[blueprint:get_id()] = blueprint
      end
   end

   for _, proxy in ipairs(proxies) do
      local blueprint = entity_map[proxy.entity_id]

      if proxy.building_id then
         local progress = blueprint:get_component('stonehearth:construction_progress')
         local building = entity_map[proxy.building_id]
         assert(building, string.format('could not find building %d in blueprint map', proxy.building_id))
         progress:set_building_entity(building);
      end
   
      -- xxx: this whole "borrow scaffolding from" thing should go away when we factor scaffolding
      -- out of the fabricator and into the build service!
      if proxy.loan_scaffolding_to then
         local cd = blueprint:add_component('stonehearth:construction_data')
         for _, borrower_id in ipairs(proxy.loan_scaffolding_to) do
            local borrower = entity_map[borrower_id]
            assert(borrower, string.format('could not find scaffolding borrower entity %d in blueprint map', borrower_id))
            cd:loan_scaffolding_to(borrower)
         end
      end
   end

   return result
end

function BuildService:set_active(building, enabled)
   local function _set_active_recursive(blueprint, enabled)
      local ec = blueprint:get_component('entity_container')  
      if ec then
         for id, child in ec:each_child() do
            _set_active_recursive(child, enabled)
         end
      end
      local cp = blueprint:get_component('stonehearth:construction_progress')
      if cp then
         cp:set_active(enabled)
      end
   end  
   _set_active_recursive(building, enabled)
end

function BuildService:set_teardown(blueprint, enabled)
   local function _set_teardown_recursive(blueprint)
      local ec = blueprint:get_component('entity_container')  
      if ec then
         for id, child in ec:each_child() do
            _set_teardown_recursive(child, enabled)
         end
      end
      local cp = blueprint:get_component('stonehearth:construction_progress')
      if cp then
         cp:set_teardown(enabled)
      end
   end  
   _set_teardown_recursive(blueprint, enabled)
end

function BuildService:_begin_construction(blueprint, location)
   -- create a new fabricator...   to... you know... fabricate
   local fabricator = self:create_fabricator_entity(blueprint)
   radiant.terrain.place_entity(fabricator, location)
end

function BuildService:create_fabricator_entity(blueprint)
   -- either you're a fabricator or you contain things which may be fabricators.  not
   -- both!
   local fabricator = radiant.entities.create_entity('stonehearth:entities:fabricator')
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
   fabricator:set_debug_text('(Fabricator for ' .. tostring(blueprint) .. ')')
   
   if blueprint:get_component('stonehearth:construction_data') then
      local name = tostring(blueprint)
      fabricator:add_component('stonehearth:fabricator')
                     :start_project(name, blueprint)
      blueprint:add_component('stonehearth:construction_progress')   
                  :set_fabricator_entity(fabricator)
   end

end

function BuildService:_init_fabricator_children(fabricator, blueprint)
   local ec = blueprint:get_component('entity_container')  
   if ec then
      for id, child in ec:each_child() do
         local fc = self:create_fabricator_entity(child)
         fabricator:add_component('entity_container'):add_child(fc)
      end
   end
end

-- THERE BE DRAGONS HERE THERE BE DRAGONS HERE THERE BE DRAGONS HERE THERE BE DRAGONS HERE 
--
-- Stuff below the line is sane and will work with multiplayer.
--
-- Stuff above this line is suspect.  The proxy upload thing is really hard to get right
-- when many people are uploading proxies, and frankly I believe it will be much more
-- error prone going forward (e.g. editing, merging, etc.)
--
-- THERE BE DRAGONS HERE THERE BE DRAGONS HERE THERE BE DRAGONS HERE THERE BE DRAGONS HERE

-- Add a new floor segment to the world.  This will try to merge with existing buildings
-- if the floor overlaps some pre-existing floor or will create a new building to hold
-- the floor
--
--    @param session - the session for the player initiating the request
--    @param response - a response object which we'll write the result into
--    @param floor_uri - the uri to type of floor we'd like to add
--    @param box - the area of the new floor segment

function BuildService:add_floor(session, response, floor_uri, box)
   local floor

   -- look for floor that we can merge into.
   local existing_floor = radiant.terrain.get_entities_in_cube(box, function(entity)
         local cd = entity:get_component('stonehearth:construction_data')
         return cd and cd:get_type() == "floor"
      end)

   if not next(existing_floor) then
      -- there was no existing floor at all. create a new building and add a floor
      -- segment to it. 
      local building = self:_create_new_building(session, box.min)
      floor = self:_add_new_floor_to_building(building, floor_uri, box)
   else
      -- we overlapped some pre-existing floor.  merge this box into that floor,
      -- potentially merging multiple buildings together!
      floor = self:_merge_overlapping_floor(existing_floor, floor_uri, box)
   end
   
   -- if we managed to create some floor, return the fabricator to the client as the
   -- new selected entity.  otherwise, return an error.
   if floor then
      local floor_fab = floor:get_component('stonehearth:construction_progress'):get_fabricator_entity()
      response:resolve({
         new_selection = floor_fab
      })
   else
      response:reject({ error = 'could not create floor' })
   end
end

-- adds a new fabricator to blueprint.  this creates a new 'stonehearth:entities:fabricator'
-- entity, adds a fabricator component to it, and wires that fabricator up to the blueprint.
-- See `_init_fabricator` for more details.
--    @param blueprint - The blueprint which needs a new fabricator.
--
function BuildService:_add_fabricator(blueprint)
   local fabricator = radiant.entities.create_entity('stonehearth:entities:fabricator')
   self:_init_fabricator(fabricator, blueprint)
   return fabricator
end

-- adds a new `blueprint` entity to the specified `building` entity at the optional
-- location.  also handles making sure the blueprint is owned by the building owner
-- and creating a fabricator for the blueprint
--
--    @param building - The building entity which will contain the blueprint
--    @param blueprint - The blueprint to be added to the building
--    @param location - (optional) a Point3 representing the offset in the building
--                      where the blueprint is located
--
function BuildService:_add_to_building(building, blueprint, location)

   -- make the owner of the blueprint the same as the owner as of the building
   blueprint:add_component('unit_info')
            :set_player_id(radiant.entities.get_player_id(building))
            :set_faction(radiant.entities.get_faction(building))


   -- add the blueprint to the building's entity container and wire up the
   -- building entity pointer in the construction_progress component.
   local cp = blueprint:add_component('stonehearth:construction_progress')
   cp:set_building_entity(building)
   radiant.entities.add_child(building, blueprint, location)

   -- if the blueprint does not yet have a fabricator, go ahead and create one
   -- now.
   local fabricator = cp:get_fabricator_entity()
   if not fabricator then
      fabricator = self:_add_fabricator(blueprint)
   end
   return fabricator
end

-- creates a new building for the owner of `sesssion` located at `origin` in
-- the world
--
--     @param session - the session of the owning player
--     @param location - a Point3 representing the position of the new building in the
--                       world
--
function BuildService:_create_new_building(session, location)
   local building = radiant.entities.create_entity('stonehearth:entities:building')
   
   -- give the building a unique name and establish ownership.
   building:add_component('unit_info')
           :set_display_name(string.format('Building No.%d', self._sv.next_building_id))
           :set_player_id(session.player_id)
           :set_faction(session.faction)
           

   self._sv.next_building_id = self._sv.next_building_id + 1
   self.__saved_variables:mark_changed()

   -- add a construction progress component.  though the building is just a container
   -- and requires no fabricator, we still need a CP component to activate and track
   -- progress.
   building:add_component('stonehearth:construction_progress')

   -- finally, put the entity on the ground at the requested location
   radiant.terrain.place_entity(building, location)
   return building
end

-- adds a new piece of floor to some existing set of floor objects contained in the
-- `existing_floor` map (keyed by entity id).  if the `box` overlaps many different
-- pieces of floor, we'll have to merge them all together into a single floor entity.
-- if those pieces of floor belong to different buildings, merge all the buildings
-- together, too!
--
--    @param existing_floor - a table of all floor segments that overlap the `box`
--    @param floor_uri - the uri to type of floor we'd like to add
--    @param box - the area of the new floor segment
--
function BuildService:_merge_overlapping_floor(existing_floor, floor_uri, box)
   local id, floor = next(existing_floor)
   local id, next_floor = next(existing_floor, id)
   if not next_floor then
      -- exactly 1 overlapping floor.  just modify the region of that floor.
      -- pretty easy
      local  region = floor:get_component('destination'):get_region()
      local origin = radiant.entities.get_world_grid_location(floor)
      region:modify(function(cursor)
            cursor:add_cube(box:translated(-origin))
         end)
      return floor
   end
end


-- create a new floor of size `box` to `building`.  it is up to the caller
-- to ensure the new floor doesn't overlap with any other floor in the
-- building!
--
--    @param building - the building to contain the new floor
--    @param floor_uri - the uri to type of floor we'd like to add
--    @param box - the area of the new floor segment
--
function BuildService:_add_new_floor_to_building(building, floor_uri, box)
   local floor = radiant.entities.create_entity(floor_uri)

   local origin = radiant.entities.get_world_grid_location(building)
   local region = _radiant.sim.alloc_region()
   region:modify(function(c)
         c:add_unique_cube(box:translated(-origin))
      end)
   floor:add_component('destination'):set_region(region)

   self:_add_to_building(building, floor)
   return floor
end

return BuildService

