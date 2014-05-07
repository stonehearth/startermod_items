local Spawner = class()
local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3

function Spawner:__init()
end

function Spawner:initialize(properties)
   local session = {
      player_id = 'game_master',
      faction = 'goblin',
      kingdom = 'stonehearth:kingdoms:golden_conquering_arm'
   }
   self._town = stonehearth.town:add_town(session)
   self._inventory = stonehearth.inventory:add_inventory(session)
   self._population = stonehearth.population:add_population(session)
   self._population:create_town_name()
   radiant.events.listen(stonehearth.object_tracker, 'stonehearth:promote', self, self._on_promote)
end

function Spawner:_on_promote(e)
   if e.entity:get_component('stonehearth:profession'):get_profession_id() == 'carpenter' then
      local explored_regions = stonehearth.terrain:get_explored_region('civ'):get()
      local region_bounds = explored_regions:get_bounds()

      local spawn_point = Point3(region_bounds.max.x + 1, 1, 1 + (region_bounds.max.y + region_bounds.min.y) / 2)

      self._goblin = self._population:create_new_citizen()
      radiant.terrain.place_entity(self._goblin, spawn_point)
      self._town:join_task_group(self._goblin, 'workers')
      self._stockpile = self._inventory:create_stockpile(spawn_point, {x=2, y=1})
      self._stockpile_comp = self._stockpile:get_component('stonehearth:stockpile')
      self._stockpile_comp:set_filter({'resource wood'})

      radiant.events.listen(self._stockpile, 'stonehearth:item_added', self, self._item_added)
   end
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
   end
end

function Spawner:_item_removed(e)


end

return Spawner
