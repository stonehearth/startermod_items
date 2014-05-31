local GoblinBrigands = class()
local EscortSquad = require 'scenarios.dynamic.goblin_brigands.escort_squad'
local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_rng()

function GoblinBrigands.can_spawn()
   return true
end

--[[ 
Goblin Brigands
]]
function GoblinBrigands:__init(saved_variables)
   self.__saved_variables = saved_variables
   self._sv = self.__saved_variables:get_data()

   if self._sv._triggered then
      if self._sv._squad then
         self._sv._squad = EscortSquad('game_master', self._sv._squad)
      end
      if not self._sv._squad or not self._sv._squad:spawned() then
         -- We were saved in a triggered state, but no goblin has been spawned.
         -- This means if you keep saving/loading before the goblin spawns, the 
         -- goblin will never spawn.  Oh well.
         self:start()
      else
         -- Triggered, and with a goblin on the move.
         self:_attach_listeners()
         self:_add_restock_task(self._sv._thief)
      end
   end
end

function GoblinBrigands:start()
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

   self:_schedule_spawn(1)
end

function GoblinBrigands:_attach_listeners()
   radiant.events.listen(self._sv._stockpile, 'stonehearth:item_added', self, self._item_added)
   radiant.events.listen_once(self._sv._squad, 'stonehearth:squad:squad_destroyed', self, self._squad_killed)
end

function GoblinBrigands:_add_restock_task(e)
   local s_comp = self._sv._stockpile:get_component('stonehearth:stockpile')
   e:get_component('stonehearth:ai')
      :get_task_group('stonehearth:work')
      :create_task('stonehearth:restock_stockpile', { stockpile = s_comp })
      :set_source(self._sv._stockpile)
      :set_name('stockpile thief task')
      :set_priority(stonehearth.constants.priorities.worker_task.RESTOCK_STOCKPILE)
      :start()
   self.__saved_variables:mark_changed()
end

function GoblinBrigands:_schedule_spawn(t)
   stonehearth.calendar:set_timer(t, function()
         self:_on_spawn()
      end)
end

function GoblinBrigands:_on_spawn()
   if not self._sv._squad then
      self._sv._squad = EscortSquad('game_master', radiant.create_datastore())
      self._sv._thief = self._sv._squad:set_escorted('stonehearth:goblin:thief')
      self._sv._squad:add_escort('stonehearth:goblin:brigand', 'stonehearth:wooden_sword')
      self._sv._squad:add_escort('stonehearth:goblin:brigand', 'stonehearth:wooden_sword')
   end

   local spawn_points = stonehearth.spawn_region_finder:find_standable_points_outside_civ_perimeter(
      self._sv._squad:get_all_entities(), self._sv._squad:get_squad_start_displacements(), 80)

   if not spawn_points then
      -- Couldn't find a spawn point, so reschedule to try again later.
      self:_schedule_spawn(rng:get_int(3600 * 0.5, 3600 * 1))
      return
   end

   self._sv._stockpile = self._inventory:create_stockpile(spawn_points[1], {x=2, y=2})
   local s_comp = self._sv._stockpile:get_component('stonehearth:stockpile')
   s_comp:set_filter({'resource wood'})
   s_comp:set_should_steal(true)

   self._sv._squad:place_squad(spawn_points[1])

   self:_attach_listeners()
   self:_add_restock_task(self._sv._thief)
   self.__saved_variables:mark_changed()
end

function GoblinBrigands:_squad_killed(e)
   radiant.events.unlisten(self._sv._stockpile, 'stonehearth:item_added', self, self._item_added)

   radiant.entities.destroy_entity(self._sv._stockpile)
   self._sv._stockpile = nil
   self._sv._suqad = nil
   self._sv._triggered = false
   self.__saved_variables:mark_changed()

   radiant.events.trigger(self, 'stonehearth:dynamic_scenario:finished')
end

function GoblinBrigands:_item_added(e)
   local s_comp = self._sv._stockpile:get_component('stonehearth:stockpile')
   if self._sv._stockpile:is_valid() and s_comp:is_full() and self._sv._thief:is_valid() then
      self._sv._thief:get_component('stonehearth:ai')
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

return GoblinBrigands
