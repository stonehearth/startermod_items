local MicroWorld = class()

local Point2 = _radiant.csg.Point2
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
   local session = {
      player_id = 'player_1',
      faction = 'civ',
      kingdom = 'stonehearth:kingdoms:ascendancy',
   }
   stonehearth.town:add_town(session)
   stonehearth.inventory:add_inventory(session)
   stonehearth.population:add_population(session)

   local region3 = _radiant.sim.alloc_region()
   region3:modify(function(r3)
      r3:add_cube(Cube3(Point3(0, -16, 0), Point3(self._size, 0, self._size), Terrain.SOIL_STRATA))
      r3:add_cube(Cube3(Point3(0,   0, 0), Point3(self._size, 1, self._size), Terrain.GRASS))
   end)

   local terrain = radiant._root_entity:add_component('terrain')
   terrain:set_tile_size(self._size)
   terrain:add_tile(Point3(-16, 0, -16), region3)
end

function MicroWorld:at(time, fn)
   return radiant.set_realtime_timer(time, fn)
end

function MicroWorld:place_tree(x, z)
   return self:place_item('stonehearth:small_oak_tree', x, z)
end

function MicroWorld:place_item(uri, x, z, player_id)
   local entity = radiant.entities.create_entity(uri)
   if player_id then
      entity:add_component('unit_info'):set_player_id(player_id)
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
   local pop = stonehearth.population:get_population('player_1')
   local citizen = pop:create_new_citizen()
   profession = profession and profession or 'stonehearth:professions:worker'


   if not string.find(profession, ':') and not string.find(profession, '/') then
      -- as a convenience for autotest writers, stick the stonehearth:profession on
      -- there if they didn't put it there to begin with
      profession = 'stonehearth:professions:' .. profession
   end
   citizen:add_component('stonehearth:profession'):promote_to(profession)

   radiant.terrain.place_entity(citizen, Point3(x, 1, z))
   return citizen
end

function MicroWorld:place_stockpile_cmd(player_id, x, z, w, h)
   w = w and w or 3
   h = h and h or 3

   local location = Point3(x, 1, z)
   local size = Point2( w, h )

   local inventory = stonehearth.inventory:get_inventory(player_id)
   inventory:create_stockpile(location, size)
end

return MicroWorld
