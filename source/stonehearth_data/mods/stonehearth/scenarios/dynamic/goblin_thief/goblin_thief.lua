local GoblinThief = class()
local rng = _radiant.csg.get_default_rng()


function GoblinThief.can_spawn()
   return true
end

--[[ 
Goblin Thief (aka Goblin Jerk) narrative:
When the camp standard is placed, we want to start gently (and then not-so-gently) harassing the player.
To wit: spawn a little thief, somewhere just outside the explored area.  And then this little guy goes back
and forth stealing wood (probably should just be from the stockpile, but for now, anywhere.)

]]
function GoblinThief:__init(saved_variables)
   self.__saved_variables = saved_variables
   self._sv = self.__saved_variables:get_data()

   if self._sv._triggered then
      if not self._sv._goblin then
         -- We were saved in a triggered state, but no goblin has been spawned.
         -- This means if you keep saving/loading before the goblin spawns, the 
         -- goblin will never spawn.  Oh well.
         self:start()
      else
         -- Triggered, and with a goblin on the move.
         self:_attach_listeners()
         self:_add_restock_task()
      end
   end
end

function GoblinThief:start()
   -- Begin hack #1: We want some reasonable place to put faction initialization; in some random scenario
   -- is likely not the correct place.
   local session = {
      player_id = 'game_master',
      faction = 'goblin',
      kingdom = 'stonehearth:kingdoms:golden_conquering_arm'
   }
   if stonehearth.town:get_town(session.player_id) == nil then
      stonehearth.town:add_town(session)
      self._inventory = stonehearth.inventory:add_inventory(session)
      self._population = stonehearth.population:add_population(session)
      self._population:create_town_name()
   else
      self._inventory = stonehearth.inventory:get_inventory(session.player_id)
      self._population = stonehearth.population:get_population(session.player_id)
   end
   -- End hack

   self._sv._triggered = true
   self.__saved_variables:mark_changed()

   self:_schedule_next_spawn(1)
end

function GoblinThief:_attach_listeners()
   radiant.events.listen(self._sv._stockpile, 'stonehearth:item_added', self, self._item_added)
   radiant.events.listen(self._sv._goblin, 'radiant:entity:pre_destroy', self, self._goblin_killed)
end

function GoblinThief:_add_restock_task()
   local s_comp = self._sv._stockpile:get_component('stonehearth:stockpile')
   self._sv._goblin:get_component('stonehearth:ai')
      :get_task_group('stonehearth:work')
      :create_task('stonehearth:restock_stockpile', { stockpile = s_comp })
      :set_source(self._sv._stockpile)
      :set_name('stockpile thief task')
      :set_priority(stonehearth.constants.priorities.worker_task.RESTOCK_STOCKPILE)
      :start()
   self.__saved_variables:mark_changed()
end

function GoblinThief:_schedule_next_spawn(t)
   stonehearth.calendar:set_timer(t, function()
         self:_on_spawn_jerk()
      end)
end

function GoblinThief:_on_spawn_jerk()
   self._sv._goblin = self._population:create_new_citizen()
   local spawn_point = stonehearth.spawn_region_finder:find_point_outside_civ_perimeter_for_entity(self._sv._goblin, 80)

   if not spawn_point then
      -- Couldn't find a spawn point, so reschedule to try again later.
      radiant.entities.destroy_entity(self._sv._goblin)
      self._sv._goblin = nil
      self:_schedule_next_spawn(rng:get_int(3600 * 0.5, 3600 * 1))
      return
   end

   self._sv._stockpile = self._inventory:create_stockpile(spawn_point, {x=2, y=1})
   local s_comp = self._sv._stockpile:get_component('stonehearth:stockpile')
   s_comp:set_filter({'resource wood'})
   s_comp:set_should_steal(true)

   radiant.terrain.place_entity(self._sv._goblin, spawn_point)

   self:_attach_listeners()
   self:_add_restock_task()
   self.__saved_variables:mark_changed()
end

function GoblinThief:_goblin_killed(e)
   radiant.events.unlisten(self._sv._stockpile, 'stonehearth:item_added', self, self._item_added)
   radiant.events.unlisten(self._sv._goblin, 'radiant:entity:pre_destroy', self, self._goblin_killed)

   radiant.entities.destroy_entity(self._sv._stockpile)
   self._sv._stockpile = nil
   self._sv._goblin = nil
   self._sv._triggered = false
   self.__saved_variables:mark_changed()

   radiant.events.trigger(self, 'stonehearth:dynamic_scenario:finished')
end

function GoblinThief:_item_added(e)
   local s_comp = self._sv._stockpile:get_component('stonehearth:stockpile')
   if self._sv._stockpile:is_valid() and s_comp:is_full() and self._sv._goblin:is_valid() then
      self._sv._goblin:get_component('stonehearth:ai')
         :get_task_group('stonehearth:work')
         :create_task('stonehearth:stockpile_arson', { 
            stockpile_comp = s_comp, 
            location = self._sv._stockpile:get_component('mob'):get_grid_location()
         })
         :set_priority(stonehearth.constants.priorities.top.WORK)
         :once()
         :start()
      self.__saved_variables:mark_changed()
   end
end

return GoblinThief
