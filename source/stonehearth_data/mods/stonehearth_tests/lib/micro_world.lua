local MicroWorld = class()

local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Terrain = _radiant.om.Terrain

function MicroWorld:__init()
   self._nextTime = 1
   self._times = {}
   self._timers = {}
   self._running = false

   radiant.events.listen('radiant.events.gameloop', self)
end

-- xxx: this timer system really should be in radiant.events.  nuke it!
MicroWorld['radiant.events.gameloop'] = function(self, time)
   if not self._running then
      self._running = true;
      table.sort(self._times);
   end
   local nextTimer = self._times[self._nextTime];
   while nextTimer ~= nil and nextTimer <= time do
      self._timers[nextTimer](time);
      self._nextTime = self._nextTime + 1
      nextTimer = self._times[self._nextTime]
   end
end

function MicroWorld:create_world()
   local region3 = _radiant.sim.alloc_region()
   local r3 = region3:modify()

   r3:add_cube(Cube3(Point3(0, -16, 0), Point3(32, 0, 32), Terrain.TOPSOIL))
   r3:add_cube(Cube3(Point3(0,   0, 0), Point3(32, 1, 32), Terrain.FOOTHILLS))

   local terrain = radiant._root_entity:add_component('terrain')
   terrain:set_zone_size(32)
   terrain:add_zone(Point3(-16, 0, -16), region3)
end

function MicroWorld:at(time, fn)
   table.insert(self._times, time);
   self._timers[time] = fn
end

function MicroWorld:place_tree(x, z)
   return self:place_item('stonehearth.medium_oak_tree', x, z)
end

function MicroWorld:place_item(uri, x, z, faction)
   local entity = radiant.entities.create_entity(uri)
   radiant.terrain.place_entity(entity, Point3(x, 1, z))
   if faction then
      entity:add_component('unit_info'):set_faction(faction)
   end
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
   local pop_service = radiant.mods.load('stonehearth').population
   local pop = pop_service:get_faction('stonehearth.factions.ascendancy')
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

   local inventory_service = radiant.mods.load('stonehearth').inventory
   local inventory = inventory_service:get_inventory(faction)
   inventory:create_stockpile(location, size)
end

function MicroWorld:create_room(faction, x, z, w, h)
   w = w and w or 3
   h = h and h or 3
   local bounds = Cube3(Point3(x, 1, z),
                                 Point3(x + w, 2, z + h))

   return radiant.mods.get_singleton('stonehearth_building').create_room(faction, x, z, w, h)
end

function MicroWorld:create_door_cmd(wall, x, y, z)
   radiant.check.is_a(wall, Wall)
   local json, obj = radiant.commands.call('radiant.commands.create_portal', wall, '//stonehearth/buildings/wooden_door', Point3(x, y, z))
   return om:get_entity(obj.entity_id)
end

function MicroWorld:start_project_cmd(blueprint)
   radiant.check.is_entity(blueprint);
   local json, obj = radiant.commands.call('radiant.commands.start_project', blueprint)
   return om:get_entity(obj.entity_id)
end

return MicroWorld
