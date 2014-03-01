local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Terrain = _radiant.om.Terrain

local TestEnvironment = class()
function TestEnvironment:__init()
   
   self._size = 32
   self._default_faction = 'civ'

   local region3 = _radiant.sim.alloc_region()
   region3:modify(function(r3)
      r3:add_cube(Cube3(Point3(0, -16, 0), Point3(self._size, 0, self._size), Terrain.SOIL_STRATA))
      r3:add_cube(Cube3(Point3(0,   0, 0), Point3(self._size, 1, self._size), Terrain.GRASS))
   end)

   local terrain = radiant._root_entity:add_component('terrain')
   terrain:set_tile_size(self._size)
   terrain:add_tile(Point3(-self._size / 2, 0, -self._size / 2), region3)

   -- listen for every entity creation event so we can tear them all down between tests
   self._all_entities = {}
   radiant.events.listen(radiant.events, 'stonehearth:entity_created', function(e)
         table.insert(self._all_entities, e.entity)
      end)
end

function TestEnvironment:clear()
   for _, entity in ipairs(self._all_entities) do
      radiant.entities.destroy_entity(entity)
   end
   self._all_entities = {}
end

function TestEnvironment:run(testfn)
   local execute = coroutine.wrap(testfn)   
end

function TestEnvironment:create_entity(x, z, uri, options)
   options = options or {}
   
   local entity = radiant.entities.create_entity(uri)
   if options.faction then
      radiant.entities.set_faction(entity, options.faction)
   end
   local location = Point3(x, 1, z)
   radiant.terrain.place_entity(entity, location)
   return entity
end

function TestEnvironment:create_person(x, z, options)
   options = options or {}
   local faction = options.faction or self._default_faction
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

function TestEnvironment:create_stockpile(x, z, options)
   options = options or {}
   
   local faction = options.faction or self._default_faction
   local size = options.size or { x = 2, y = 2}

   local location = Point3(x, 1, z)
   local inventory = stonehearth.inventory:get_inventory(faction)

   return inventory:create_stockpile(location, size)
end

return TestEnvironment
