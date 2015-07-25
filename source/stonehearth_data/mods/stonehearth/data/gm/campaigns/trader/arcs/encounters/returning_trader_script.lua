local ReturningTrader = class()
local rng = _radiant.csg.get_default_rng()

--[[
   Returning Trader narrative
   The returning trader asks for something the town does not currently have, and will return for it in
   a number of days. In exchange, they will give the town something relatively rare. 

   Though the trader could theoretically ask for anything, this is a great opportunity to ask for
   something from a craftsman. 

   This is also a great opportunty to give the town with something significant, like a new 
   recipe or crop to plant. Though this is not currently enabled because silkweed is 
   available from the beginning it can be enabled by adding something like this to the rewards: 
      "stonehearth:crops:silkweed_crop" : {
         "type" : "crop"
      },
]]

function ReturningTrader:_load_data(data)
   self._sv._trade_info = data

   self._sv._want_table = {}
   for uri, data in pairs(self._sv._trade_info.wants) do
      table.insert(self._sv._want_table, uri)
   end

   self._sv._reward_table = {}
   for uri, data in pairs(self._sv._trade_info.rewards) do
      table.insert(self._sv._reward_table, uri)
   end
end

function ReturningTrader:start(ctx, data)
   self._sv._player_id = ctx.player_id
   self._sv.trader_entity = radiant.entities.create_entity()
   radiant.entities.set_player_id(self._sv.trader_entity, 'trader')
   self._sv.leased_items = {}

   self:_load_data(data)

   local want_uri, want_count = self:_get_desired_items()
   local days = rng:get_int(1, self._sv._trade_info.max_days_before_return)
   local reward_uri, reward_count, reward_type = self:_get_reward_items()

   self._sv._trade_data = {
      want_uri = want_uri, 
      want_count = want_count, 
      num_days = days, 
      reward_uri = reward_uri, 
      reward_count = reward_count, 
      reward_type = reward_type
   }

   local message = self:_make_message(self._sv._trade_info.greeting .. self._sv._trade_info.trade_details)

   --Send the notice to the bulletin service
   self._sv._bulletin = stonehearth.bulletin_board:post_bulletin(self._sv._player_id)
      :set_ui_view('StonehearthGenericBulletinDialog')
      :set_callback_instance(self)
      :set_data({
         title = self._sv._trade_info.title,
         message = message,
         accepted_callback = "_on_accepted",
         declined_callback = "_on_declined",
      })

   --Make sure it times out if we don't get to it
   local wait_duration = self._sv._trade_info.expiration_timeout
   self:_create_timer(wait_duration)
end

--- Returns the URI of the desired item, and the number of desired items 
function ReturningTrader:_get_desired_items()
   local random_want_index = rng:get_int(1, #self._sv._want_table)
   local wanted_item_uri = self._sv._want_table[random_want_index]
   local proposed_number = rng:get_int(self._sv._trade_info.wants[wanted_item_uri].min, self._sv._trade_info.wants[wanted_item_uri].max)

   local inventory = stonehearth.inventory:get_inventory(self._sv._player_id)
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
   local random_reward_index =  rng:get_int(1, #self._sv._reward_table)
   local reward_item_uri = self._sv._reward_table[random_reward_index]
   local reward_item_data = self._sv._trade_info.rewards[reward_item_uri]
   local proposed_number = 1
   if reward_item_data.type == 'object' then
      proposed_number = rng:get_int(reward_item_data.min, reward_item_data.max)
   end
   if reward_item_data.type == 'crop' then
      --Make sure we only proceed if this settlement doesn't already have this crop
      --TODO: Remove this check once we implement bags of seeds
      local session = {
         player_id = self._sv._player_id,
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
      reward_name = string.gsub(self._sv._trade_info.crop_name, '__reward_name__', reward_name)
   end

   message_composite = string.gsub(message_composite, '__want_number__', self._sv._trade_data.want_count)
   message_composite = string.gsub(message_composite, '__want_item__', want_name)
   message_composite = string.gsub(message_composite, '__num_days__', self._sv._trade_data.num_days)
   message_composite = string.gsub(message_composite, '__reward_number__', self._sv._trade_data.reward_count)
   message_composite = string.gsub(message_composite, '__reward_item__', reward_name)
   message_composite = string.gsub(message_composite, '__crop_name__', crop_name)

   local hours_remaing = 0
   if self._sv.timer then
      hours_remaing = stonehearth.calendar:get_remaining_time(self._sv.timer, 'd')
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
   self._sv.timer = stonehearth.calendar:set_timer("ReturningTrader timer", duration, radiant.bind(self, '_timer_callback'))
end

function ReturningTrader:_timer_callback()
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
end

function ReturningTrader:_cancel_bulletin(bulletin)
   local bulletin_id = bulletin:get_id()
   stonehearth.bulletin_board:remove_bulletin(bulletin_id)
   self:_stop_timer()
end

function ReturningTrader:_stop_timer(timer)
   if self._sv.timer then
      self._sv.timer:destroy()
      self._sv.timer = nil
   end
end

--- When the user accepts the trader's challenge
function ReturningTrader:_on_accepted()
   self._sv._bulletin = nil
   self:_stop_timer()

   --make a timer to express how long to wait before the caravan returns
   self._sv._wait_time = self._sv._trade_data.num_days
   self:_create_timer(self._sv._wait_time .. 'd')

   --Put up a non-dismissable notification to keep track of the time remaining
   --Send the notice to the bulletin service
   local title = self:_make_message(self._sv._trade_info.waiting_title)
   local message = self:_make_message(self._sv._trade_info.waiting_text)
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
   if not self._sv.hourly_timer then
      self._sv.hourly_timer = stonehearth.calendar:set_interval("ReturningTrader on_hourly", '1h', radiant.bind(self, '_on_hourly'))
   end
end

function ReturningTrader:_on_hourly()
   if self._sv._waiting_for_return then
      local title = self:_make_message(self._sv._trade_info.waiting_title)
      local message = self:_make_message(self._sv._trade_info.waiting_text)
      self._sv._bulletin:set_data({
         title = title,
         message = message
      })
   else
      if self._sv.hourly_timer then
         self._sv.hourly_timer:destroy()
         self._sv.hourly_timer = nil
      end
   end
end

function ReturningTrader:_on_declined()
   self._sv._approach_bulletin = nil
   self._sv._success_bulletin = nil
   self._sv._failure_bulletin = nil
   if self._sv.timer then
      self._sv.timer:destroy()
      self._sv.timer = nil
   end
   self:_stop_timer()
   radiant.events.trigger(self, 'stonehearth:dynamic_scenario:finished')
end

-- The trader's deal was accepted, and now he's back. 
function ReturningTrader:_make_return_trip()
   local town_has_goods = false
   local inventory = stonehearth.inventory:get_inventory(self._sv._player_id)   
   
   --Want the iconic form, if you have it. 
   local want_item_real_uri = self._sv._trade_data.want_uri
   local data = radiant.entities.get_component_data(self._sv._trade_data.want_uri, 'stonehearth:entity_forms')
   if data then
      want_item_real_uri = data.iconic_form
   end

   local inventory_data_for_item = inventory:get_items_of_type(want_item_real_uri)
   if inventory_data_for_item and inventory_data_for_item.count >= self._sv._trade_data.want_count then
      --We have the items desired. Reserve them all/as many as possible
      town_has_goods = self:_reserve_items(inventory_data_for_item, self._sv._trade_data.want_count)
   end

   if town_has_goods then
      --Make the success message
      local message = self:_make_message(self._sv._trade_info.return_greetings_positive)
      --Make the success bulletin
      self._sv._bulletin = stonehearth.bulletin_board:post_bulletin(self._sv._player_id)
         :set_ui_view('StonehearthGenericBulletinDialog')
         :set_callback_instance(self)
         :set_data({
            title = self._sv._trade_info.return_title,
            message = message,
            accepted_callback = "_on_success_accepted",
            declined_callback = "_on_declined",
         })
   else
      --Make the failure message
      local message = self:_make_message(self._sv._trade_info.return_greetings_negative)
      --Make the failure bulletin
      self._sv._bulletin = stonehearth.bulletin_board:post_bulletin(self._sv._player_id)
         :set_ui_view('StonehearthGenericBulletinDialog')
         :set_callback_instance(self)
         :set_data({
            title = self._sv._trade_info.return_title,
            message = message,
            ok_callback = "_on_declined"
         })
   end

   --Make a timer for either (so the bulletins both time out)
   local wait_duration = self._sv._trade_info.expiration_timeout
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
   self:_stop_timer()

   --If the new thing was a crop, make an informational bulletin
   if self._sv._trade_data.reward_type == 'crop' then
      local message = self:_make_message(self._sv._trade_info.new_crop_confirmation_message)
      self._sv._bulletin = stonehearth.bulletin_board:post_bulletin(self._sv._player_id)
         :set_ui_view('StonehearthGenericBulletinDialog')
         :set_callback_instance(self)
         :set_data({
            title = self._sv._trade_info.new_crop_confirmation_title,
            message = message,
            ok_callback = "_on_declined"
         })
      --Make a timer for either (so the bulletins both time out)
      local wait_duration = self._sv._trade_info.expiration_timeout
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
   local banner = town:get_banner()
   local drop_origin = banner and radiant.entities.get_world_grid_location(banner)
   if not drop_origin then
      return
   end

   --If the reward type was an object, make the new objects
   if self._sv._trade_info.rewards[self._sv._trade_data.reward_uri].type == 'object' then
      local uris = {}
      uris[self._sv._trade_data.reward_uri] = self._sv._trade_data.reward_count
      --TODO: attach a brief particle effect to the new stuff
      radiant.entities.spawn_items(uris, drop_origin, 1, 3, { owner = self._sv._player_id })
   elseif self._sv._trade_info.rewards[self._sv._trade_data.reward_uri].type == 'crop' then
      local session = {
         player_id = self._sv._player_id,
      }
      stonehearth.farming:add_crop_type(session, self._sv._trade_data.reward_uri)
   end

   --TODO: account for new crops and new recipes
end

--TODO: START HERE FRIDAY TESTING THIS CODE!!!

return ReturningTrader