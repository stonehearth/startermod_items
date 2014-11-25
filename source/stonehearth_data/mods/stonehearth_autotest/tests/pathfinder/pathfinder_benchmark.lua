local Point3 = _radiant.csg.Point3

local pathfinder_benchmark = {}

local PATHFINDING_TIME = 20000
local DEBUG_FOLLOW_SHOW_PATHS = false

function pathfinder_benchmark.basic_a_star(autotest)
   local world = autotest.env.get_world()
   local still_searching = {}

   -- pause the game and wait a bit
   -- Remember the current game speed.  We need to turn it down to 1 during the timing critical part of
   -- the test
   local current_speed
   if not DEBUG_FOLLOW_SHOW_PATHS then
      _radiant.call('radiant:game:set_game_speed', 0.1)
      autotest:sleep(100)
   end

   local endpoint = autotest.env:create_entity(world.endpoint.x, world.endpoint.z, 'stonehearth:small_oak_tree')
   autotest:log('destination is @ %s', radiant.entities.get_world_grid_location(endpoint))

   local people = {}
   for i, coord in pairs(world.people) do
      people[i] = {
         person = autotest.env:create_person(coord.x, coord.z, { job = 'shepherd' }),
         coord = coord,
      }
   end

   local count = 0
   local start_time = radiant.get_realtime();

   for _, entry in pairs(people) do
      local person = entry.person
      local location = Point3(entry.coord.x, 10, entry.coord.z)
      radiant.terrain.place_entity(person, location)
      local start = radiant.entities.get_world_grid_location(person)

      local function start_pathfinder()
         local job
         if DEBUG_FOLLOW_SHOW_PATHS then
            job = autotest.ai:compel(person, 'stonehearth:goto_entity', {
                  entity = endpoint
               })
         else
            job = autotest.ai:compel(person, 'stonehearth:find_path_to_entity', {
                  destination = endpoint
               })
         end
         job:done(function()
               if not autotest:is_finished() then
                  autotest:log('solved search originating @ %s', start)
                  count = count + 1
                  start_pathfinder()
               end
            end)
      end
      start_pathfinder()
   end

   autotest:log('sleeping...')
   autotest:sleep(PATHFINDING_TIME)
   local duration = radiant.get_realtime() - start_time

   autotest:log('solved %d paths in %.2f seconds ( %.2f PPS ).', count, duration, count / duration)
   autotest:success()
end

return pathfinder_benchmark
