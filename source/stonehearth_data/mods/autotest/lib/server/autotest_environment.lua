local Cube3   = _radiant.csg.Cube3
local Point3  = _radiant.csg.Point3
local Point3f = _radiant.csg.Point3f
local Terrain = _radiant.om.Terrain

local WORLD_SIZE = 32
local DEFAULT_FACTION = 'civ'

local _all_entities = {}

local env = {}
function env.create_world()
   local region3 = _radiant.sim.alloc_region()
   region3:modify(function(r3)
      r3:add_cube(Cube3(Point3(0, -16, 0), Point3(WORLD_SIZE, 0, WORLD_SIZE), Terrain.SOIL_STRATA))
      r3:add_cube(Cube3(Point3(0,   0, 0), Point3(WORLD_SIZE, 1, WORLD_SIZE), Terrain.GRASS))
   end)

   local terrain = radiant._root_entity:add_component('terrain')
   terrain:set_tile_size(WORLD_SIZE)
   terrain:add_tile(Point3(-WORLD_SIZE / 2, 0, -WORLD_SIZE / 2), region3)

   -- listen for every entity creation event so we can tear them all down between tests
   radiant.events.listen(radiant.events, 'stonehearth:entity:post_create', function(e)
         table.insert(_all_entities, e.entity)
      end)

   env.town = stonehearth.town:get_town(DEFAULT_FACTION)
end

function env.clear()
   for _, entity in ipairs(_all_entities) do
      radiant.entities.destroy_entity(entity)
   end
   _all_entities = {}
   
   -- move the camera to a decent spot
   local CAMERA_POSITION = Point3f(11, 16, 39)
   local CAMERA_LOOK_AT = Point3f(3.5, 1, 12.5)
   autotest.ui.move_camera(CAMERA_POSITION, CAMERA_LOOK_AT)
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
      local papi = radiant.mods.require(string.format('stonehearth.professions.%s.%s', options.profession, options.profession))
      papi.promote(entity)
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
   local faction = options.faction or DEFAULT_FACTION
   local culture = 'stonehearth:factions:ascendancy' -- ascendancy is not a faction.  it's a.. culture or something

   local pop = stonehearth.population:get_faction(faction, culture)
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
   local inventory = stonehearth.inventory:get_inventory(faction)

   return inventory:create_stockpile(location, size)
end

return env