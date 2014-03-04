local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
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
end

function env.clear()
   for _, entity in ipairs(_all_entities) do
      radiant.entities.destroy_entity(entity)
   end
   _all_entities = {}
end

function env.create_entity(x, z, uri, options)
   options = options or {}
   
   local entity = radiant.entities.create_entity(uri)
   if options.faction then
      radiant.entities.set_faction(entity, options.faction)
   end
   local location = Point3(x, 1, z)
   radiant.terrain.place_entity(entity, location)
   return entity
end

function env.create_person(x, z, options)
   options = options or {}
   local faction = options.faction or DEFAULT_FACTION
   local culture = 'stonehearth:factions:ascendancy' -- ascendancy is not a faction.  it's a.. culture or something

   local pop = stonehearth.population:get_faction(faction, culture)
   local person = pop:create_new_citizen()

   if options.profession then
      local papi = radiant.mods.require(string.format('stonehearth.professions.%s.%s', options.profession, options.profession))
      papi.promote(person)
   end
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
