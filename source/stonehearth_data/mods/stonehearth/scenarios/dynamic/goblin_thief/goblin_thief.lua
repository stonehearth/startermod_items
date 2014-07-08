local GoblinThief = class()
local rng = _radiant.csg.get_default_rng()


function GoblinThief:can_spawn()
   return true
end

--[[ 
Goblin Thief (aka Goblin Jerk) narrative:
When the camp standard is placed, we want to start gently (and then not-so-gently) harassing the player.
To wit: spawn a little thief, somewhere just outside the explored area.  And then this little guy goes back
and forth stealing wood (probably should just be from the stockpile, but for now, anywhere.)
]]
function GoblinThief:initialize()
   --ID of the player who the goblin is attacking, not the player ID of the DM
   self._sv.player_id = 'player_1'
end

function GoblinThief:restore()
   if self._sv.__scenario:is_running() then
      radiant.events.listen_once(radiant, 'radiant:game_loaded', function(e)
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
         end)
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

   self:_schedule_next_spawn(1)
end

function GoblinThief:_attach_listeners()
   radiant.events.listen(self._sv._stockpile, 'stonehearth:item_added', self, self._item_added)
   radiant.events.listen(self._sv._goblin, 'radiant:entity:pre_destroy', self, self._goblin_killed)
   radiant.events.listen(self._sv._goblin, 'stonehearth:carry_block:carrying_changed', self, self._theft_event)
end

--The goblin thief heads towards the town, and if he sees stuff, he will grab it
function GoblinThief:_add_restock_task()
   local s_comp = self._sv._stockpile:get_component('stonehearth:stockpile')
   self._sv._goblin:get_component('stonehearth:ai')
      :get_task_group('stonehearth:work')
      :create_task('stonehearth:restock_stockpile', { stockpile = s_comp })
      :set_source(self._sv._stockpile)
      :set_name('stockpile thief task')
      :set_priority(stonehearth.constants.priorities.goblins.HOARD)
      :start()

   local town = stonehearth.town:get_town(self._sv.player_id)
   local banner = town:get_banner()
   self._sv._goblin:get_component('stonehearth:ai')
      :get_task_group('stonehearth:work')
      :create_task('stonehearth:goto_entity', { entity = banner })
      :set_source(self._sv._stockpile)
      :set_name('introduce self task')
      :set_priority(stonehearth.constants.priorities.goblins.RUN_TOWARDS_SETTLEMENT)
      :once()
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
   --TODO: Right now the filter is broken Why???
   --s_comp:set_filter({'resource wood'})

   radiant.terrain.place_entity(self._sv._goblin, spawn_point)

   self:_attach_listeners()
   self:_add_restock_task()
   self.__saved_variables:mark_changed()
end

function GoblinThief:_goblin_killed(e)
   radiant.events.unlisten(self._sv._stockpile, 'stonehearth:item_added', self, self._item_added)
   radiant.events.unlisten(self._sv._goblin, 'radiant:entity:pre_destroy', self, self._goblin_killed)
   radiant.events.unlisten(self._sv._goblin, 'stonehearth:carry_block:carrying_changed', self, self._theft_event)

   radiant.entities.destroy_entity(self._sv._stockpile)
   self._sv._stockpile = nil
   self._sv._goblin = nil

    if self._sv.notification_bulletin then
      local bulletin_id = self._sv.notification_bulletin:get_id()
      stonehearth.bulletin_board:remove_bulletin(bulletin_id)
      self._sv.notification_bulletin = nil
   end

   self.__saved_variables:mark_changed()

   radiant.events.trigger(self, 'stonehearth:dynamic_scenario:finished')
end

function GoblinThief:_theft_event(e)
   if not self._sv.bulletin_fired then
      local message_data = radiant.resources.load_json('stonehearth:scenarios:goblin_thief').scenario_data
      self._sv.title = message_data.title
      local message_index = rng:get_int(1, #message_data.quote)
      self._sv.message = message_data.quote[message_index] .. ' --' .. radiant.entities.get_name(self._sv._goblin)
      
      --Send the notice to the bulletin service.
      self._sv.notification_bulletin = stonehearth.bulletin_board:post_bulletin(self._sv.player_id)
           :set_callback_instance(self)
           :set_type('alert')
           :set_data({
               title = self._sv.title,
               message = self._sv.message,
               zoom_to_entity = self._sv._goblin,
           })

      self._sv.bulletin_fired = true

      --Add something to the event log:
      stonehearth.events:add_entry(self._sv.title .. ': ' .. self._sv.message)
   end
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

function GoblinThief:_on_ok(e)
end

return GoblinThief
