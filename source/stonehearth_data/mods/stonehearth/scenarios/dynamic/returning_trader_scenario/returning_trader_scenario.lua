local ReturningTrader = class()
local rng = _radiant.csg.get_default_rng()

--[[
   Returning Trader narrative
   The returning trader asks for something the town does not currently have, and will return for it in
   a number of days. In exchange, they will give the town something relatively rare. 

   Though the trader could theoretically ask for anything, this is a great opportunity to ask for
   something from a craftsman. 

   This is also a great opportunty to give the town with something significant, like a new 
   recipe or crop to plant. 
]]

-- The returning trader will only spawn if there is at least one carpenter in the population
-- and if the score is above a certain threshhold
function ReturningTrader:can_spawn()
   local score_data = stonehearth.score:get_scores_for_player(self._sv._player_id):get_score_data()
   local score_threshold = self._trade_data.score_threshold
   local curr_score = 0
   if score_data.net_worth and score_data.net_worth.total_score then
      curr_score = score_data.net_worth.total_score
   end
   if curr_score < score_threshold then
      return false
   end

   local pop = stonehearth.population:get_population(self._sv._player_id)
   local citizens = pop:get_citizens()
   for citizen_id, citizen_entity in pairs(citizens) do
      local profession = citizen_entity:get_component('stonehearth:profession')
      --TODO: test that this is how the URI actually looks
      if profession and profession:get_profession_uri() == 'stonehearth:professions:carpenter' then
         return true
      end
   end

   --There were no carpenters, return false
   return false
end

function ReturningTrader:initialize()
   --TODO: have to get this from the service
   self._sv._player_id = 'player_1'
   local town = stonehearth.town:get_town( self._sv._player_id)
   self._sv._kingdom = town:get_kingdom()

   --Make a dummy entity to hold the lease on desired items
   --TODO: replace with the actual caravan entity once they move in the world
   self._sv.trader_entity = radiant.entities.create_entity()   
   self._sv.leased_items = {}

   self:_load_data()
end

function ReturningTrader:restore()
   self:_load_data()

   --This is boilerplate across all scenarios. Way to factor it out?
   --If we made an expire timer then we're waiting for the player to acknowledge the traveller
   --Start a timer that will expire at that time
   if self._sv.timer_expiration then
      radiant.events.listen(radiant, 'radiant:game_loaded', function(e)
         local duration = self._sv.timer_expiration - stonehearth.calendar:get_elapsed_time()
         self:_create_timer(duration)
         return radiant.events.UNLISTEN
      end)
   end
   if self._sv._waiting_for_return then
      radiant.events.listen(stonehearth.calendar, 'stonehearth:hourly', self, self._on_hourly)
   end
end

function ReturningTrader:_load_data()
   self._trade_data = radiant.resources.load_json('stonehearth:scenarios:returning_trader').scenario_data

   self._want_table = {}
   for uri, data in pairs(self._trade_data.wants) do
      table.insert(self._want_table, uri)
   end

   self._reward_table = {}
   for uri, data in pairs(self._trade_data.rewards) do
      table.insert(self._reward_table, uri)
   end
end

function ReturningTrader:start()
   local want_uri, want_count = self:_get_desired_items()
   local days = rng:get_int(1, self._trade_data.max_days_before_return)
   local reward_uri, reward_count, reward_type = self:_get_reward_items()

   self._sv._trade_data = {
      want_uri = want_uri, 
      want_count = want_count, 
      num_days = days, 
      reward_uri = reward_uri, 
      reward_count = reward_count, 
      reward_type = reward_type
   }

   local message = self:_make_message(self._trade_data.greeting .. self._trade_data.trade_details)

   --Send the notice to the bulletin service
   self._sv._bulletin = stonehearth.bulletin_board:post_bulletin(self._sv._player_id)
      :set_ui_view('StonehearthGenericBulletinDialog')
      :set_callback_instance(self)
      :set_data({
         title = self._trade_data.title,
         message = message,
         accepted_callback = "_on_accepted",
         declined_callback = "_on_declined",
      })

   --Make sure it times out if we don't get to it
   local wait_duration = self._trade_data.expiration_timeout
   self:_create_timer(wait_duration)
end

--- Returns the URI of the desired item, and the number of desired items 
function ReturningTrader:_get_desired_items()
   local random_want_index = rng:get_int(1, #self._want_table)
   local wanted_item_uri = self._want_table[random_want_index]
   local proposed_number = rng:get_int(self._trade_data.wants[wanted_item_uri].min, self._trade_data.wants[wanted_item_uri].max)

   local inventory = stonehearth.inventory:get_inventory(self._sv.player_id)
   local inventory_data_for_item = inventory:get_items_of_type(wanted_item_uri)
   if inventory_data_for_item and inventory_data_for_item.count > 0 then
      if proposed_number < inventory_data_for_item.count then
         proposed_number = inventory_data_for_item.count + 2
      end
   end
   return wanted_item_uri, proposed_number
end

--- Returns data about the reward item
function ReturningTrader:_get_reward_items()
   local random_reward_index =  rng:get_int(1, #self._reward_table)
   local reward_item_uri = self._reward_table[random_reward_index]
   local reward_item_data = self._trade_data.rewards[reward_item_uri]
   local proposed_number = 1
   if reward_item_data.type == 'object' then
      proposed_number = rng:get_int(reward_item_data.min, reward_item_data.max)
   end
   if reward_item_data.type == 'crop' then
      --Make sure we only proceed if this settlement doesn't already have this crop
      --TODO: Remove this check once we implement bags of seeds
      local session = {
         player_id = self._sv._player_id,
         kingdom = self._sv._kingdom
      } 
      if stonehearth.farming:has_crop_type(session, reward_item_uri) then
         return self:_get_reward_items()
      end
   end
   return reward_item_uri, proposed_number, reward_item_data.type
end

--- Returns a message with all the right text
function ReturningTrader:_make_message(message_composite)
   local town_name = stonehearth.town:get_town(self._sv._player_id):get_town_name()
   message_composite = string.gsub(message_composite, '__town_name__', town_name)

   local want_name = self:_get_user_visible_name(self._sv._trade_data.want_uri)
   local reward_name =  self:_get_user_visible_name(self._sv._trade_data.reward_uri)
   local crop_name = ""
   if self._sv._trade_data.reward_type == 'crop' then
      crop_name = reward_name
      reward_name = string.gsub(self._trade_data.crop_name, '__reward_name__', reward_name)
   end

   message_composite = string.gsub(message_composite, '__want_number__', self._sv._trade_data.want_count)
   message_composite = string.gsub(message_composite, '__want_item__', want_name)
   message_composite = string.gsub(message_composite, '__num_days__', self._sv._trade_data.num_days)
   message_composite = string.gsub(message_composite, '__reward_number__', self._sv._trade_data.reward_count)
   message_composite = string.gsub(message_composite, '__reward_item__', reward_name)
   message_composite = string.gsub(message_composite, '__crop_name__', crop_name)

   local hours_remaing = 0
   if self._timer then
      hours_remaing = self._timer:get_remaining_time('h')
      hours_remaing = math.floor(hours_remaing)
      message_composite = string.gsub(message_composite, '__hour_counter__',  hours_remaing)
   end
   return message_composite
end

--TODO: move this to an i18n service, duplicated from simple_carava
--Ok, maybe it's time to make that string service you were talking about, Tony. :) --sdee
function ReturningTrader:_get_user_visible_name(uri)
   local item_data = radiant.resources.load_json(uri)
   if item_data.components.unit_info then
      return item_data.components.unit_info.name
   elseif item_data.components['stonehearth:iconic_form'] then
      local full_sized_data =  radiant.resources.load_json(item_data.components['stonehearth:iconic_form'].full_sized_entity)
      return full_sized_data.components.unit_info.name
   end
end

--Creates the timers used in this function.
function ReturningTrader:_create_timer(duration)
   self._timer = stonehearth.calendar:set_timer(duration, function() 
      --We're the timer to cancel the some bulletin
      if self._sv._bulletin then
         self:_cancel_bulletin(self._sv._bulletin)
      end
      if self._sv._waiting_for_return then
         --We're the timer to bring back the caravan. 
         --self:_stop_timer()
         self._sv._waiting_for_return = false
         self:_make_return_trip()
      end
   end)
   self._sv.timer_expiration = self._timer:get_expire_time()
end

function ReturningTrader:_cancel_bulletin(bulletin)
   local bulletin_id = bulletin:get_id()
   stonehearth.bulletin_board:remove_bulletin(bulletin_id)
   self:_stop_timer()
end

function ReturningTrader:_stop_timer(timer)
   self._timer = nil
   self._sv.timer_expiration = nil
end

--- When the user accepts the trader's challenge
function ReturningTrader:_on_accepted()
   self._sv._bulletin = nil
   if self._timer then
      self._timer:destroy()
   end
   self:_stop_timer()

   --make a timer to express how long to wait before the caravan returns
   self._sv._wait_time = self._sv._trade_data.num_days
   self:_create_timer('24h')--self._sv._wait_time .. 'd'--)

   --Put up a non-dismissable notification to keep track of the time remaining
   --Send the notice to the bulletin service
   local title = self:_make_message(self._trade_data.waiting_title)
   local message = self:_make_message(self._trade_data.waiting_text)
   self._sv._bulletin = stonehearth.bulletin_board:post_bulletin(self._sv._player_id)
      :set_ui_view('StonehearthGenericBulletinDialog')
      :set_callback_instance(self)
      :set_close_on_handle(false)
      :set_data({
         title = title,
         message = message
      })
   --register a callback every hour so we can
   self._sv._waiting_for_return = true
   radiant.events.listen(stonehearth.calendar, 'stonehearth:hourly', self, self._on_hourly)
end

function ReturningTrader:_on_hourly()
   if self._sv._waiting_for_return then
      local title = self:_make_message(self._trade_data.waiting_title)
      local message = self:_make_message(self._trade_data.waiting_text)
      self._sv._bulletin:set_data({
         title = title,
         message = message
      })
   else
      return radiant.events.UNLISTEN
   end
end

function ReturningTrader:_on_declined()
   self._sv._approach_bulletin = nil
   self._sv._success_bulletin = nil
   self._sv._failure_bulletin = nil
   if self._timer then
      self._timer:destroy()
   end
   self:_stop_timer()
   radiant.events.trigger(self, 'stonehearth:dynamic_scenario:finished')
end

-- The trader's deal was accepted, and now he's back. 
function ReturningTrader:_make_return_trip()
   local town_has_goods = false
   local inventory = stonehearth.inventory:get_inventory(self._sv.player_id)   
   local inventory_data_for_item = inventory:get_items_of_type(self._sv._trade_data.want_uri)
   if inventory_data_for_item and inventory_data_for_item.count >= self._sv._trade_data.want_count then
      --We have the items desired. Reserve them all/as many as possible
      town_has_goods = self:_reserve_items(inventory_data_for_item, self._sv._trade_data.want_count)
   end

   if town_has_goods then
      --Make the success message
      local message = self:_make_message(self._trade_data.return_greetings_positive)
      --Make the success bulletin
      self._sv._bulletin = stonehearth.bulletin_board:post_bulletin(self._sv._player_id)
         :set_ui_view('StonehearthGenericBulletinDialog')
         :set_callback_instance(self)
         :set_data({
            title = self._trade_data.return_title,
            message = message,
            accepted_callback = "_on_success_accepted",
            declined_callback = "_on_declined",
         })
   else
      --Make the failure message
      local message = self:_make_message(self._trade_data.return_greetings_negative)
      --Make the failure bulletin
      self._sv._bulletin = stonehearth.bulletin_board:post_bulletin(self._sv._player_id)
         :set_ui_view('StonehearthGenericBulletinDialog')
         :set_callback_instance(self)
         :set_data({
            title = self._trade_data.return_title,
            message = message,
            ok_callback = "_on_declined"
         })
   end

   --Make a timer for either (so the bulletins both time out)
   local wait_duration = self._trade_data.expiration_timeout
   self:_create_timer(wait_duration)
end

--- Given inventory data for a type of item, reserve N of those items
--  TODO: copied from simple_caravan. Maybe move this and unreserve to inventory service?
function ReturningTrader:_reserve_items(inventory_data_for_item, num_desired)
   local num_reserved = 0
   for id, item in pairs(inventory_data_for_item.items) do
      if item then
         local leased = stonehearth.ai:acquire_ai_lease(item, self._sv.trader_entity)
         if leased then 
            table.insert(self._sv.leased_items, item)
            num_reserved = num_reserved + 1
            if num_reserved >= num_desired then
               return true
            end
         end
      end
   end
   --if we got here, we didn't reserve enough to satisfy demand. 
   if num_reserved <= 0 then
      self:_unreserve_items()
      return false
   end
   return true
end

--- Remove our lien on all the items the trader was eyeing
function ReturningTrader:_unreserve_items()
   for i, item in ipairs(self._sv.leased_items) do
      stonehearth.ai:release_ai_lease(item, self._sv.trader_entity)
   end
   self._sv.leased_items = {}
end

function ReturningTrader:_on_success_accepted()
   self:_accept_trade()
   if self._timer then
      self._timer:destroy()
   end
   self:_stop_timer()

   --If the new thing was a crop, make an informational bulletin
   if self._sv._trade_data.reward_type == 'crop' then
      local message = self:_make_message(self._trade_data.new_crop_confirmation_message)
      self._sv._bulletin = stonehearth.bulletin_board:post_bulletin(self._sv._player_id)
         :set_ui_view('StonehearthGenericBulletinDialog')
         :set_callback_instance(self)
         :set_data({
            title = self._trade_data.new_crop_confirmation_title,
            message = message,
            ok_callback = "_on_declined"
         })
      --Make a timer for either (so the bulletins both time out)
      local wait_duration = self._trade_data.expiration_timeout
      self:_create_timer(wait_duration)
   else
      --If it wasn't a crop thing, then just finish
      radiant.events.trigger(self, 'stonehearth:dynamic_scenario:finished')
   end
end

--TODO: instead of doing this, the trader should pick them up and haul them off
function ReturningTrader:_take_items()
   for i, item in ipairs(self._sv.leased_items) do
      stonehearth.ai:release_ai_lease(item, self._sv.caravan_entity)
      radiant.entities.destroy_entity(item)
   end
   self._sv.leased_items = {}
end

--- Call this if the player accepts the trade
function ReturningTrader:_accept_trade()
   --TODO: go through the reserved items and nuke them all
   self:_take_items()

   --Add the new items to the space near the banner
   local town = stonehearth.town:get_town(self._sv._player_id)
   local banner_entity = town:get_banner()

   --If the reward type was an object, make the new objects
   if self._trade_data.rewards[self._sv._trade_data.reward_uri].type == 'object' then
      for i=1, self._sv._trade_data.reward_count do
         local target_location = radiant.entities.pick_nearby_location(banner_entity, 3)
         local item = radiant.entities.create_entity(self._sv._trade_data.reward_uri)   
         radiant.entities.set_player_id(item, self._sv._player_id)
         radiant.terrain.place_entity(item, target_location)

         --TODO: attach a brief particle effect to the new stuff
      end
   elseif  self._trade_data.rewards[self._sv._trade_data.reward_uri].type == 'crop' then
      local session = {
         player_id = self._sv._player_id,
         kingdom = self._sv._kingdom
      }
      stonehearth.farming:add_crop_type(session, self._sv._trade_data.reward_uri)
   end

   --TODO: account for new crops and new recipes
end

--TODO: START HERE FRIDAY TESTING THIS CODE!!!

return ReturningTrader