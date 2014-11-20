local GoblinRaidingCamp = class()
local EscortSquad = require 'scenarios.dynamic.goblin_brigands.escort_squad'
local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_rng()

function GoblinRaidingCamp:can_spawn()
   return false --true
end

--[[ 
Goblin Brigands
]]
function GoblinRaidingCamp:__init(saved_variables)
end

function GoblinRaidingCamp:initialize()
   --ID of the player who the goblin is attacking, not the player ID of the DM
   self._sv.player_id = 'player_1'
end

function GoblinRaidingCamp:restore()
   if self._sv.__scenario:is_running() then
      radiant.events.listen_once(radiant, 'radiant:game_loaded', function(e)
            if self._sv._squad then
               self._sv._squad = EscortSquad('goblins', self._sv._squad)
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
         end)
   end
end

function GoblinRaidingCamp:start()
   -- Begin hack #1: We want some reasonable place to put faction initialization; in some random scenario
   -- is likely not the correct place.
   local session = {
      player_id = 'goblins',
   }
   
   -- the camp's stockpile
   --self._sv._stockpile = self._inventory:create_stockpile({0,0,0}, {x=2, y=1})

   --self._sv.services:place_entity('stonehearth:monsters:goblins:goblin_tent', 0, 0, true)
   
   return

   --[[
   if stonehearth.town:get_town(session.player_id) == nil then
      stonehearth.town:add_town(session)
      self._inventory = stonehearth.inventory:add_inventory(session)
      self._population = stonehearth.population:add_population(session, 'stonehearth:kingdoms:goblin')
      self._population:create_town_name()
   else
      self._inventory = stonehearth.inventory:get_inventory(session.player_id)
      self._population = stonehearth.population:get_population(session.player_id)
   end
   -- End hack
   ]]

   
end

function GoblinRaidingCamp:_attach_listeners()
   self._item_added_listener = radiant.events.listen(self._sv._stockpile, 'stonehearth:stockpile:item_added', self, self._item_added)
   radiant.events.listen_once(self._sv._squad, 'stonehearth:squad:squad_destroyed', self, self._squad_killed)
   self._destroy_listener = radiant.events.listen(self._sv._thief, 'radiant:entity:pre_destroy', self, self._thief_killed)
   self._carrying_listener = radiant.events.listen(self._sv._thief, 'stonehearth:carry_block:carrying_changed', self, self._theft_event)
end

function GoblinRaidingCamp:_add_restock_task(e)
   local s_comp = self._sv._stockpile:get_component('stonehearth:stockpile')
   e:get_component('stonehearth:ai')
      :get_task_group('stonehearth:work')
      :create_task('stonehearth:restock_stockpile', { stockpile = self._sv._stockpile })
      :set_source(self._sv._stockpile)
      :set_name('stockpile thief task')
      :set_priority(stonehearth.constants.priorities.goblins.HOARD)
      :start()

   local town = stonehearth.town:get_town(self._sv.player_id)
   local banner = town:get_banner()
   e:get_component('stonehearth:ai')
      :get_task_group('stonehearth:work')
      :create_task('stonehearth:goto_entity', { entity = banner })
      :set_source(self._sv._stockpile)
      :set_name('introduce self task')
      :set_priority(stonehearth.constants.priorities.goblins.RUN_TOWARDS_SETTLEMENT)
      :once()
      :start()

   self.__saved_variables:mark_changed()
end

function GoblinRaidingCamp:_schedule_spawn(t)
   stonehearth.calendar:set_timer(t, function()
         self:_on_spawn()
      end)
end

--Determine the # of brigands based on the wealth of the town
function GoblinRaidingCamp:_on_spawn()
   if not self._sv._squad then
      self._sv._squad = EscortSquad('goblins', radiant.create_datastore())
      self._sv._thief = self._sv._squad:set_escorted('stonehearth:goblin:thief')

      --Set the thief to only attack when attacked.
      --If he's not being attacked, he will go collect stuff
      stonehearth.combat:set_stance(self._sv._thief, 'defensive')

      --When he's attacked, he will use this wooden sword
      local weapon = radiant.entities.create_entity('stonehearth:weapons:wooden_sword')
      radiant.entities.equip_item(self._sv._thief, weapon)

      --Pick how many escorts
      local score_data = stonehearth.score:get_scores_for_player(self._sv.player_id):get_score_data()
      local num_escorts = 1
      if score_data.net_worth and score_data.net_worth.total_score then
         local proposed_escorts = score_data.net_worth.total_score / 1000
         if proposed_escorts > num_escorts then
            num_escorts = proposed_escorts
         end
      end

      for i = 1, num_escorts do 
         self._sv._squad:add_escort('stonehearth:goblin:brigand', 'stonehearth:weapons:wooden_sword')
      end
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
   --TODO: right now the filter is broken. Why???
   --s_comp:set_filter({'resource wood'})

   self._sv._squad:place_squad(spawn_points[1])

   self:_attach_listeners()
   self:_add_restock_task(self._sv._thief)
   self.__saved_variables:mark_changed()
end

function GoblinRaidingCamp:_squad_killed(e)
   self._item_added_listener:destroy()
   self._item_added_listener = nil

   radiant.entities.destroy_entity(self._sv._stockpile)
   self._sv._stockpile = nil
   self._sv._suqad = nil
   self.__saved_variables:mark_changed()

   radiant.events.trigger(self, 'stonehearth:dynamic_scenario:finished')
end

function GoblinRaidingCamp:_thief_killed(e)
   self._carrying_listener:destroy()
   self._carrying_listener = nil

   self._destroy_listener:destroy()
   self._destroy_listener = nil

   radiant.events.trigger_async(stonehearth.linear_combat, 'stonehearth:goblin_killed')

   if self._sv.notification_bulletin then
      local bulletin_id = self._sv.notification_bulletin:get_id()
      stonehearth.bulletin_board:remove_bulletin(bulletin_id)
      self._sv.notification_bulletin = nil
   end

   self.__saved_variables:mark_changed()
end

function GoblinRaidingCamp:_theft_event(e)
   if not self._sv.bulletin_fired then
      local message_data = radiant.resources.load_json('stonehearth:scenarios:goblin_brigands').scenario_info
      self._sv.title = message_data.title
      local message_index = rng:get_int(1, #message_data.quote)
      self._sv.message = message_data.quote[message_index] .. ' --' .. radiant.entities.get_name(self._sv._thief)
      
      --Send the notice to the bulletin service.
      self._sv.notification_bulletin = stonehearth.bulletin_board:post_bulletin(self._sv.player_id)
           :set_callback_instance(self)
           :set_type('alert')
           :set_data({
               title = self._sv.title,
               message = self._sv.message,
               zoom_to_entity = self._sv._thief,
           })

      self._sv.bulletin_fired = true

      --Add something to the event log:
      stonehearth.events:add_entry(self._sv.title .. ': ' .. self._sv.message)
   end
end


function GoblinRaidingCamp:_item_added(e)
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

return GoblinRaidingCamp
