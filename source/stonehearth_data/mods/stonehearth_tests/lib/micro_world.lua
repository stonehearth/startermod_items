local MicroWorld = class()

local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

function MicroWorld:__init(size)
   self._nextTime = 1
   self._running = false
   self._session = {
      player_id = 'player_1',
   }

   if not size then
      size = 32
   end
   self._size = size
end

function MicroWorld:get_session()
   return self._session
end

function MicroWorld:create_world()
   local session = self:get_session()
   stonehearth.town:add_town(session)
   stonehearth.inventory:add_inventory(session)
   stonehearth.population:add_population(session, 'stonehearth:kingdoms:ascendancy')
   -- xxx, oh lawd. These gets are required to intialize the terrain visible and explored regions.
   -- refactoring is required.
   stonehearth.terrain:get_visible_region(session.player_id) 
   stonehearth.terrain:get_explored_region(session.player_id)

   assert(self._size % 2 == 0)
   local half_size = self._size / 2

   local block_types = radiant.terrain.get_block_types()

   local region3 = Region3()
   region3:add_cube(Cube3(Point3(0, -2, 0), Point3(self._size, 0, self._size), block_types.bedrock))
   region3:add_cube(Cube3(Point3(0, 0, 0), Point3(self._size, 9, self._size), block_types.soil_dark))
   region3:add_cube(Cube3(Point3(0, 9, 0), Point3(self._size, 10, self._size), block_types.grass))
   region3 = region3:translated(Point3(-half_size, 0, -half_size))

   radiant._root_entity:add_component('terrain')
                           :add_tile(region3)
end

function MicroWorld:at(time, fn)
   return radiant.set_realtime_timer(time, fn)
end

function MicroWorld:place_tree(x, z)
   return self:place_item('stonehearth:small_oak_tree', x, z)
end

function MicroWorld:place_item(uri, x, z, player_id, options)
   local entity = radiant.entities.create_entity(uri)
   if player_id then
      entity:add_component('unit_info'):set_player_id(player_id)
   end
   radiant.terrain.place_entity(entity, Point3(x, 1, z), options)
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

function MicroWorld:place_citizen(x, z, job)
   local pop = stonehearth.population:get_population('player_1')
   local citizen = pop:create_new_citizen()
   job = job or 'stonehearth:jobs:worker'

   if not string.find(job, ':') and not string.find(job, '/') then
      -- as a convenience for autotest writers, stick the stonehearth:job on
      -- there if they didn't put it there to begin with
      job = 'stonehearth:jobs:' .. job
   end
   citizen:add_component('stonehearth:job'):promote_to(job)

   radiant.terrain.place_entity(citizen, Point3(x, 1, z))
   return citizen
end

function MicroWorld:place_stockpile_cmd(player_id, x, z, w, h)
   w = w and w or 3
   h = h and h or 3

   local location = Point3(x, 1, z)
   local size = Point2( w, h )

   local inventory = stonehearth.inventory:get_inventory(player_id)
   return inventory:create_stockpile(location, size)
end

return MicroWorld
