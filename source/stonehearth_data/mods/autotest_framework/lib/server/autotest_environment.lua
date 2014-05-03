local Cube3   = _radiant.csg.Cube3
local Point3  = _radiant.csg.Point3
local Point3f = _radiant.csg.Point3f
local Terrain = _radiant.om.Terrain

local WORLD_SIZE = 64
local DEFAULT_FACTION = 'civ'

local _all_entities = {}

local env = {}
function env.create_world()
   env.session = {
      player_id = 'player_1',
      faction = 'civ',
      kingdom = 'stonehearth:kingdoms:ascendancy',
   }
   stonehearth.town:add_town(env.session)
   stonehearth.inventory:add_inventory(env.session)
   stonehearth.population:add_population(env.session)

   local region3 = _radiant.sim.alloc_region()
   region3:modify(function(r3)
      r3:add_cube(Cube3(Point3(0, -16, 0), Point3(WORLD_SIZE, 0, WORLD_SIZE), Terrain.SOIL_STRATA))
      r3:add_cube(Cube3(Point3(0,   0, 0), Point3(WORLD_SIZE, 1, WORLD_SIZE), Terrain.GRASS))
   end)

   local terrain = radiant._root_entity:add_component('terrain')
   terrain:set_tile_size(WORLD_SIZE)
   terrain:add_tile(Point3(-WORLD_SIZE / 2, 0, -WORLD_SIZE / 2), region3)

   -- listen for every entity creation event so we can tear them all down between tests
   radiant.events.listen(radiant, 'radiant:entity:post_create', function(e)
         local entity = e.entity
         local id = entity:get_id()
         _all_entities[id] = entity
      end)

   env.town = stonehearth.town:get_town(env.session.player_id)
end

function env.clear()
   for id, entity in pairs(_all_entities) do
      radiant.entities.destroy_entity(entity)      
   end
   _all_entities = {}
   
   -- move the camera to a decent spot
   local CAMERA_POSITION = Point3f(11, 16, 39)
   local CAMERA_LOOK_AT = Point3f(3.5, 1, 12.5)
   autotest_framework.ui.move_camera(CAMERA_POSITION, CAMERA_LOOK_AT)
end

function env.get_town()
   return env.town
end

local function apply_options_to_entity(entity, options)
   if options.faction then
      radiant.entities.set_faction(entity, options.faction)
   end
   if options.attributes then
      for name, value in pairs(options.attributes) do
         entity:get_component('stonehearth:attributes'):set_attribute(name, value)
      end
   end
   if options.profession then
      local profession = options.profession
      if not string.find(profession, ':') and not string.find(profession, '/') then
         -- as a convenience for autotest writers, stick the stonehearth:profession on
         -- there if they didn't put it there to begin with
         profession = 'stonehearth:professions:' .. profession
      end
      entity:add_component('stonehearth:profession'):promote_to(profession)
   end   
end

function env.create_entity(x, z, uri, options)
   local entity = radiant.entities.create_entity(uri)
   apply_options_to_entity(entity, options or {})
   
   local location = Point3(x, 1, z)
   radiant.terrain.place_entity(entity, location)
   return entity
end

function env.create_entity_cluster(x, y, w, h, uri, options)
   local entities = {}
   for i=x,x+w do
      for j=y,y+h do
         table.insert(entities, env.create_entity(i, j, uri, options))
      end
   end
   return entities
end

function env.create_person(x, z, options)
   options = options or {}
   local player_id = options.player_id or 'player_1'

   local pop = stonehearth.population:get_population(player_id)
   local person = pop:create_new_citizen()
   apply_options_to_entity(person, options)

   local location = Point3(x, 1, z)
   radiant.terrain.place_entity(person, location)
   return person
end

function env.create_stockpile(x, z, options)
   options = options or {}
   
   local faction = options.faction or DEFAULT_FACTION
   local size = options.size or { x = 2, y = 2}

   local location = Point3(x, 1, z)
   local inventory = stonehearth.inventory:get_inventory(env.session.player_id)

   return inventory:create_stockpile(location, size)
end

return env
