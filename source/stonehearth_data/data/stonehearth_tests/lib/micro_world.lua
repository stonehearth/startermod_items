local MicroWorld = class()

local RadiantIPoint3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Terrain = _radiant.om.Terrain

function MicroWorld:__init()
   self._nextTime = 1
   self._times = {}
   self._timers = {}
   self._running = false;

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
   radiant.terrain.add_cube(Cube3(Point3(-16, -16, -16), Point3(16, 0, 16), Terrain.TOPSOIL))
   radiant.terrain.add_cube(Cube3(Point3(-16,   0, -16), Point3(16, 1, 16), Terrain.GRASS))
end

function MicroWorld:at(time, fn)
   table.insert(self._times, time);
   self._timers[time] = fn
end

function MicroWorld:place_tree(x, z)
   return self:place_item('/stonehearth_trees/entities/oak_tree/medium_oak_tree', x, z)
end

function MicroWorld:place_item(name, x, z)
   local tree = radiant.entities.create_entity(name)
   radiant.terrain.place_entity(tree, RadiantIPoint3(x, 1, z))
   return tree
end

function MicroWorld:place_entity(x, z, name)
   local e = radiant.entities.create_entity(name)
   radiant.terrain.place_entity(e, RadiantIPoint3(x, 1, z))
   return e
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

function MicroWorld:place_citizen(x, z, profession, profession_info)
   local citizen = radiant.mods.load_api('/stonehearth_human_race/').create_entity()
   profession = profession and profession or 'worker'
   local profession = radiant.mods.load_api('/stonehearth_' .. profession .. '_class/').promote(citizen, profession_info)
   radiant.terrain.place_entity(citizen, RadiantIPoint3(x, 1, z))
   return citizen
end

function MicroWorld:place_stockpile_cmd(faction, x, z, w, h)
   w = w and w or 3
   h = h and h or 3

   local location = RadiantIPoint3(x, 1, z)
   local size = { w, h }

   local inventory = radiant.mods.load_api('/stonehearth_inventory/').get_inventory(faction)
   inventory:create_stockpile(location, size)
end

function MicroWorld:create_room(faction, x, z, w, h)
   w = w and w or 3
   h = h and h or 3
   local bounds = Cube3(RadiantIPoint3(x, 1, z),
                                 RadiantIPoint3(x + w, 2, z + h))

   return radiant.mods.require('/stonehearth_building').create_room(faction, x, z, w, h)
end

function MicroWorld:create_door_cmd(wall, x, y, z)
   radiant.check.is_a(wall, Wall)
   local json, obj = radiant.commands.call('radiant.commands.create_portal', wall, '//stonehearth/buildings/wooden_door', RadiantIPoint3(x, y, z))
   return om:get_entity(obj.entity_id)
end

function MicroWorld:start_project_cmd(blueprint)
   radiant.check.is_entity(blueprint);
   local json, obj = radiant.commands.call('radiant.commands.start_project', blueprint)
   return om:get_entity(obj.entity_id)
end

return MicroWorld
