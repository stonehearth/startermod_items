local SimpleCaravan = class()
local rng = _radiant.csg.get_default_rng()

--TOO MANY TODOS!  In order to make this work, we also have to fix the inventory service. 

function SimpleCaravan:can_spawn()
   --TODO: ask Chris how to satisfy all of these conditions
   --TODO: account for only having the caravan come by every N days?
   --TODO: don't send the caravan during the winter?
   --TODO: don't send the caravan when there's been a lot of violence in the area recently
   --Only spawn the caravan if there is at least 1 stockpile
   local inventory = stonehearth.inventory:get_inventory(self._sv.player_id)
   local storage = inventory:get_all_storage()
   local num_stockpiles = 0
   for id, storage in pairs(storage) do
      if storage and not storage:get_component('stonehearth:stockpile'):is_outbox() then
         num_stockpiles = num_stockpiles + 1
         break
      end
   end  
   local should_run = num_stockpiles > 0
   return should_run
end

--[[
   Simple Caravan narrative
   Every <TODO!> a caravan from your kingdom comes by your town.
   Based on its observations of your resources, it offers to make a trade.
   TODO: Make more complex caravans based on the different kingdoms; maybe different scenarios
]]

function SimpleCaravan:initialize()
   self:_load_trade_data()

   self._sv.leased_items = {}
   self._sv.trade_data = {}

   --TODO: get this from the calling service
   self._sv.player_id = 'player_1'
   
   --Make a dummy entity to hold the lease on desired items
   --TODO: replace with the actual caravan entity once they move in the world
   self._sv.caravan_entity = radiant.entities.create_entity()   
end

function SimpleCaravan:restore()
   self:_load_trade_data()
end

function SimpleCaravan:_load_trade_data()
   --Read in the possible trades
   self._trade_data = radiant.resources.load_json('stonehearth:scenarios:simple_caravan').scenario_data
   self._all_trades = self._trade_data.trades
   self._trade_item_table = {}
   for trade_item, data in pairs(self._all_trades) do
      table.insert(self._trade_item_table, trade_item)
   end
end

function SimpleCaravan:start()
   --For now, just spawn the caravan immediately
   self:_on_spawn_caravan()

   --TODO: spawn some population events only during the day
   --self:_schedule_next_spawn()
end

--- The caravan will spawn sometime during the day, a few hours after this triggers
--  Caravans do not spawn at night, so if it's night, add some padding
--  For testing, have the caravan spawn immediately
function SimpleCaravan:_schedule_next_spawn()
    --local hours_till_spawn = 0

   -- TODO: Ask Chris where to put this logic. 
   -- In other words if we want it to spawn at night, where is the best place to 
   -- put that check? 
   --[[
   local curr_time = stonehearth.calendar:get_time_and_date()
   local night_padding = 0
   if curr_time.hour < 6 or curr_time.hour > 20 then
      night_padding = rng:get_int(10, 12)
   end
   hours_till_spawn = night_padding + rng:get_int(0, 2)
   --]]
   --stonehearth.calendar:set_timer(hours_till_spawn..'h', function()
   --      self:_on_spawn_caravan()
   --   end)
end

function SimpleCaravan:_on_spawn_caravan()
   -- Get the object that the trader wants
   local town_has, town_quantity = self:_select_desired_item()
   if town_has and town_quantity then
      --Get the object the trader is willing to trade
      local caravan_has, caravan_quantity = self:_select_trade_item(town_has)

      self._sv.trade_data = {
         town_has = town_has,
         town_quantity = town_quantity, 
         caravan_has = caravan_has, 
         caravan_quantity = caravan_quantity
      }

      local message_composite = self:_make_message()

      --Send the notice to the bulletin service. Should be parametrized by player
      self._sv.caravan_bulletin = stonehearth.bulletin_board:post_bulletin(self._sv.player_id)
           :set_ui_view('StonehearthGenericBulletinDialog')
           :set_callback_instance(self)
           :set_data({
               title = self._trade_data.title,
               message = message_composite,
               accepted_callback = "_on_accepted",
               declined_callback = "_on_declined",
           })

      --Make sure it times out if we don't get to it
      local wait_duration = self._trade_data.expiration_timeout
      self:_create_timer(wait_duration)
   end
end

function SimpleCaravan:_make_message()
   local message_composite = self._trade_data.message

   local caravan_has = self:_get_user_visible_name(self._sv.trade_data.caravan_has)
   local town_has = self:_get_user_visible_name(self._sv.trade_data.town_has)
   local town_name = stonehearth.town:get_town(self._sv.player_id):get_town_name()

   message_composite = string.gsub(message_composite, '__town_name__', town_name)
   message_composite = string.gsub(message_composite, '__your_quantity__', self._sv.trade_data.town_quantity)
   message_composite = string.gsub(message_composite, '__your_item__', town_has)
   message_composite = string.gsub(message_composite, '__my_quantity__', self._sv.trade_data.caravan_quantity)
   message_composite = string.gsub(message_composite, '__my_item__', caravan_has)

   return message_composite
end

--TODO: move this to an i18n service
function SimpleCaravan:_get_user_visible_name(uri)
   local item_data = radiant.resources.load_json(uri)
   if item_data.components.unit_info then
      return item_data.components.unit_info.name
   elseif item_data.components['stonehearth:placeable_item_proxy'] then
      local full_sized_data =  radiant.resources.load_json(item_data.components['stonehearth:placeable_item_proxy'].full_sized_entity)
      return full_sized_data.components.unit_info.name
   end
end

function SimpleCaravan:_select_trade_item(want_item_id)
   local random_trade_index = rng:get_int(1, #self._trade_item_table)
   local trade_item_id = self._trade_item_table[random_trade_index]
   if trade_item_id == want_item_id then
      if random_trade_index > 1 then
         trade_item_id =  self._trade_item_table[random_trade_index - 1]
      else 
         trade_item_id =  self._trade_item_table[random_trade_index + 1]
      end
   end
   local quantity = rng:get_int(self._all_trades[trade_item_id].min, self._all_trades[trade_item_id].max)
   return trade_item_id, quantity
end

--- Picks an item from the trade list that the trader wants. 
--  If the town has any of those items in its inventory, the trade will want less than
--  or equal to the nubmer of items that it can reserve
--  @returns item_id, num_desired. If either is nil, we didn't find anything we wanted
function SimpleCaravan:_select_desired_item()
   --Get our inventory all ready to query
   --local inventory_tracker = stonehearth.object_tracker:get_inventory_tracker(self._sv.player_id)
   
   local random_want_index = rng:get_int(1, #self._trade_item_table)
   for i=1, #self._trade_item_table do
      local item_id = self._trade_item_table[random_want_index]
      local num_desired = rng:get_int(self._all_trades[item_id].min, self._all_trades[item_id].max)

      --TODO: add this function to the inventory tracker
      local inventory_data_for_item = stonehearth.inventory:get_items_of_type(item_id, self._sv.player_id)-- inventory_tracker:get_data_for_type(item_id)
      if inventory_data_for_item and inventory_data_for_item.count > 0 then
         if inventory_data_for_item.count < num_desired then
            num_desired = inventory_data_for_item.count
         end
         --TODO: If the trader wants this item, then pre-emptively reserve the ones the trader wants.
         --If they can be reserved, return. If they can't be reserved, continue and pick something else.
         --If there was even 1 to reserve, then lower the needed trade to that one. 
         local did_reserve = self:_reserve_items(inventory_data_for_item, num_desired)
         if did_reserve then
            num_desired = #self._sv.leased_items
            return item_id, num_desired
         end
      end

      --If there is no inventory item for this proposed trade, move on to the next trade
      random_want_index = random_want_index + 1
      if random_want_index > #self._trade_item_table then
         random_want_index = 1
      end
   end

   --If we got here, that means there was no trade appropriate and the merchant has to leave unsatisfied
   return nil, nil
end

--- Given inventory data for a type of item, reserve N of those items
function SimpleCaravan:_reserve_items(inventory_data_for_item, num_desired)
   local num_reserved = 0
   for id, item in pairs(inventory_data_for_item.items) do
      if item then
         local leased = stonehearth.ai:acquire_ai_lease(item, self._sv.caravan_entity)
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
function SimpleCaravan:_unreserve_items()
   for i, item in ipairs(self._sv.leased_items) do
      stonehearth.ai:release_ai_lease(item, self._sv.caravan_entity)
   end
   self._sv.leased_items = {}
end

--TODO: instead of doing this, the trader should pick them up and haul them off
function SimpleCaravan:_take_items()
   for i, item in ipairs(self._sv.leased_items) do
      stonehearth.ai:release_ai_lease(item, self._sv.caravan_entity)
      radiant.entities.destroy_entity(item)
   end
   self._sv.leased_items = {}
end

--- Call this if the player accepts the trade
function SimpleCaravan:_accept_trade()
   --TODO: go through the reserved items and nuke them all
   self:_take_items()

   --Add the new items to the space near the banner
   local town = stonehearth.town:get_town(self._sv.player_id)
   local banner_entity = town:get_banner()

   for i=1, self._sv.trade_data.caravan_quantity do
      local target_location = radiant.entities.pick_nearby_location(banner_entity, 3)
      local item = radiant.entities.create_entity(self._sv.trade_data.caravan_has)   
      radiant.entities.set_player_id(item, self._sv.player_id)
      radiant.terrain.place_entity(item, target_location)

      --TODO: attach a brief particle effect to the new stuff
   end
end

--- Call this if the player rejects the trade
function SimpleCaravan:_reject_trade()
   self:_unreserve_items()
end

--- Only actually spawn the object after the user clicks OK
function SimpleCaravan:_on_accepted()
   self:_accept_trade()
   if self._timer then
      self._timer:destroy()
   end
   self:_stop_timer()
   radiant.events.trigger(self, 'stonehearth:dynamic_scenario:finished')
end

function SimpleCaravan:_on_declined()
   self:_reject_trade()
   if self._timer then
      self._timer:destroy()
   end
   self:_stop_timer()
   radiant.events.trigger(self, 'stonehearth:dynamic_scenario:finished')
end

function SimpleCaravan:_create_timer(duration)
   self._timer = stonehearth.calendar:set_timer(duration, function() 
      if self._sv.caravan_bulletin then
         local bulletin_id = self._sv.caravan_bulletin:get_id()
         stonehearth.bulletin_board:remove_bulletin(bulletin_id)
         self:_stop_timer()
      end
   end)
   self._sv.timer_expiration = self._timer:get_expire_time()
end

function SimpleCaravan:_stop_timer()
   self._timer = nil
   self._sv.timer_expiration = nil
end


return SimpleCaravan