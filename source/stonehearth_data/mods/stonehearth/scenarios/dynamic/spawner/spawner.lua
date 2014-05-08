local Spawner = class()
local rng = _radiant.csg.get_default_rng()
local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3

--[[ 
Goblin Theif (aka Goblin Jerk) narrative:
When the camp standard is placed, we want to start gently (and then not-so-gently) harassing the player.
To wit: spawn a little thief, somewhere just outside the explored area.  And then this little guy goes back
and forth stealing wood (probably should just be from the stockpile, but for now, anywhere.)

]]
function Spawner:__init()
end

function Spawner:initialize(properties)
   -- Hack #1: We want some reasonable place to put faction initialization; in some random scenario
   -- is likely not the correct place.
   local session = {
      player_id = 'game_master',
      faction = 'goblin',
      kingdom = 'stonehearth:kingdoms:golden_conquering_arm'
   }
   self._town = stonehearth.town:add_town(session)
   self._inventory = stonehearth.inventory:add_inventory(session)
   self._population = stonehearth.population:add_population(session)
   self._population:create_town_name()

   radiant.events.listen(radiant, 'radiant:entity:post_create', function (e)
      if e.entity:get_uri() == 'stonehearth:camp_standard' then
         -- Once the camp-standard has been placed, wait some time before spawning the first
         -- jerk.
         self:_schedule_next_spawn(rng:get_int(3600 * 1, 3600 * 1))
         return radiant.events.UNLISTEN
      end
   end)
end

function Spawner:_schedule_next_spawn(t)
   stonehearth.calendar:set_timer(t, function()
         self:_on_spawn_jerk()
      end)
end


-- Next to do: make this not suck:
-- --choose an interesting unexplored point
-- --error check _everything_
-- --make it a util so other scenarios can use it.
function Spawner:_choose_spawn_point()
   local explored_regions = stonehearth.terrain:get_explored_region('civ'):get()
   local region_bounds = explored_regions:get_bounds()
   return Point3(region_bounds.max.x + 1, 1, 1 + (region_bounds.max.y + region_bounds.min.y) / 2)
end

function Spawner:_on_spawn_jerk()
   local spawn_point = self:_choose_spawn_point()

   self._goblin = self._population:create_new_citizen()
   radiant.terrain.place_entity(self._goblin, spawn_point)
   self._town:join_task_group(self._goblin, 'workers')
   
   self._stockpile = self._inventory:create_stockpile(spawn_point, {x=2, y=1})
   self._stockpile_comp = self._stockpile:get_component('stonehearth:stockpile')
   self._stockpile_comp:set_filter({'resource wood'})
   self._stockpile_comp:set_should_steal(true)

   radiant.events.listen(self._stockpile, 'stonehearth:item_added', self, self._item_added)
   radiant.events.listen(self._goblin, 'radiant:entity:pre_destroy', self, self._goblin_killed)
end

function Spawner:_goblin_killed(e)
   radiant.events.unlisten(self._stockpile, 'stonehearth:item_added', self, self._item_added)
   radiant.events.unlisten(self._goblin, 'radiant:entity:pre_destroy', self, self._goblin_killed)

   radiant.entities.destroy_entity(self._stockpile)
   self._stockpile = nil
   self._stockpile_comp = nil
   self._goblin = nil

   self:_schedule_next_spawn(rng:get_int(3600 * 1, 3600 * 1))
end

function Spawner:_item_added(e)
   if self._stockpile:is_valid() and self._stockpile_comp:is_full() and self._goblin:is_valid() then
      self._goblin:get_component('stonehearth:ai')
         :get_task_group('stonehearth:work')
         :create_task('stonehearth:stockpile_arson', { 
            stockpile_comp = self._stockpile_comp, 
            location = self._stockpile:get_component('mob'):get_grid_location()
         })
         :set_priority(stonehearth.constants.priorities.top.WORK)
         :once()
         :start()
      return radiant.events.UNLISTEN
   end
end

return Spawner
