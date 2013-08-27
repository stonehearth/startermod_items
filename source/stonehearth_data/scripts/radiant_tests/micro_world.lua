local MicroWorld = class()

local cairo = require 'lcairo'
local log = require 'radiant.core.log'
local md = require 'radiant.core.md'
local om = require 'radiant.core.om'
local sh = require 'radiant.core.sh'
local ch = require 'radiant.core.ch'
local check = require 'radiant.core.check'

function MicroWorld:__init()
   self._nextTime = 1
   self._times = {}
   self._timers = {}
   self._running = false;
   
   md:register_msg_handler_instance('radiant.test.micro_world', self)
   md:listen('radiant.events.gameloop', self)
end

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
   om:get_terrain():add_cube(Cube3(Point3(-16, -16, -16), Point3(16, 0, 16), Terrain.TOPSOIL))
   om:get_terrain():add_cube(Cube3(Point3(-16,   0, -16), Point3(16, 1, 16), Terrain.GRASS))
end

function MicroWorld:at(time, fn)
   table.insert(self._times, time);
   self._timers[time] = fn
end

function MicroWorld:place_tree(x, z)
   return self:place_item('module://stonehearth/resources/oak_tree/medium_oak_tree', x, z)
end

function MicroWorld:place_item(name, x, z)
   local tree = om:create_entity(name)
   om:place_on_terrain(tree, Point3(x, 1, z))
   return tree
end

function MicroWorld:place_entity(x, z, name)
   local e = om:create_entity(name)
   om:place_on_terrain(e, Point3(x, 1, z))
   return e
end

function MicroWorld:place_item_cluster(name, x, z, w, h)
   w = w and w or 3
   h = h and h or 3
   for i = x, x+w-1 do
      for j = z, z+h-1 do
         self:place_item('module://stonehearth/resources/oak_tree/oak_log', i, j)
      end
   end
end

function MicroWorld:place_citizen(x, z, profession)
   return sh:create_citizen(Point3(x, 1, z), profession)
end

function MicroWorld:place_stockpile_cmd(x, z, w, h)
   w = w and w or 3
   h = h and h or 3
   local bounds = RadiantBounds3(Point3(x, 1, z),
                                 Point3(x + w, 2, z + h))
   return ch:call('radiant.commands.create_stockpile', bounds)
end

function MicroWorld:create_room_cmd(x, z, w, h)
   w = w and w or 3
   h = h and h or 3
   local bounds = RadiantBounds3(Point3(x, 1, z),
                                 Point3(x + w, 2, z + h))
   local json, obj = ch:call('radiant.commands.create_room', bounds)
   return om:get_entity(obj.entity_id)
end

function MicroWorld:create_door_cmd(wall, x, y, z)
   check:is_a(wall, Wall)
   local json, obj = ch:call('radiant.commands.create_portal', wall, 'module://stonehearth/buildings/wooden_door', Point3(x, y, z))
   return om:get_entity(obj.entity_id)
end

function MicroWorld:harvest_cmd(entity)
   check:is_entity(entity);
   check:verify(om:has_component(entity, 'resource_node'))
   
   log:info('sending harvest command to %d.', entity:get_id())
   return ch:call('radiant.commands.harvest', entity)
end

function MicroWorld:start_project_cmd(blueprint)
   check:is_entity(blueprint);
   local json, obj = ch:call('radiant.commands.start_project', blueprint)
   return om:get_entity(obj.entity_id)
end

return MicroWorld
