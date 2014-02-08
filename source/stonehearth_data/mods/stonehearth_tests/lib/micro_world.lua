local MicroWorld = class()

local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Terrain = _radiant.om.Terrain

function MicroWorld:__init(size)
   self._nextTime = 1
   self._running = false

   if not size then
      size = 32
   end
   self._size = size
end

function MicroWorld:create_world()
   local region3 = _radiant.sim.alloc_region()
   region3:modify(function(r3)
      r3:add_cube(Cube3(Point3(0, -16, 0), Point3(self._size, 0, self._size), Terrain.SOIL))
      r3:add_cube(Cube3(Point3(0,   0, 0), Point3(self._size, 1, self._size), Terrain.GRASS))
   end)

   local terrain = radiant._root_entity:add_component('terrain')
   terrain:set_tile_size(self._size)
   terrain:add_tile(Point3(-16, 0, -16), region3)
end

function MicroWorld:at(time, fn)
   radiant.set_timer(time, fn)
end

function MicroWorld:place_tree(x, z)
   return self:place_item('stonehearth:small_oak_tree', x, z)
end

function MicroWorld:place_item(uri, x, z, faction)
   local entity = radiant.entities.create_entity(uri)
   if faction then
      entity:add_component('unit_info'):set_faction(faction)
   end
   radiant.terrain.place_entity(entity, Point3(x, 1, z))
   return entity
end

function MicroWorld:place_item_cluster(uri, x, z, w, h)
   w = w and w or 3
   h = h and h or 3
   for i = x, x+w-1 do
      for j = z, z+h-1 do
         self:place_item(uri, i, j)
      end
   end
end

function MicroWorld:place_citizen(x, z, profession, data)
   local pop = stonehearth.population:get_faction('civ', 'stonehearth:factions:ascendancy')
   local citizen = pop:create_new_citizen()
   profession = profession and profession or 'worker'

   -- this is totally gross!!
   local profession_api = radiant.mods.require(string.format('stonehearth.professions.%s.%s', profession, profession))
   profession_api.promote(citizen, data)

   radiant.terrain.place_entity(citizen, Point3(x, 1, z))
   return citizen
end

function MicroWorld:place_stockpile_cmd(faction, x, z, w, h)
   w = w and w or 3
   h = h and h or 3

   local location = Point3(x, 1, z)
   local size = { w, h }

   local inventory = stonehearth.inventory:get_inventory(faction)
   inventory:create_stockpile(location, size)
end

return MicroWorld
