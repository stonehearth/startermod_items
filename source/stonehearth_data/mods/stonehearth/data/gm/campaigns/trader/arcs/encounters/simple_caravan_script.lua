local SimpleCaravan = class()
local rng = _radiant.csg.get_default_rng()

function SimpleCaravan:initialize()
end

function SimpleCaravan:start(ctx, data)
   self._sv.player_id = ctx.player_id
   self._sv.leased_items = {}
   self._sv.trade_data = {}


   self._sv._trade_info = data
   self._sv._all_trades = data.trades
   self._sv._trade_item_table = {}
   for trade_item, data in pairs(self._sv._all_trades ) do
      table.insert(self._sv._trade_item_table, trade_item)
   end

   --Make a dummy entity to hold the lease on desired items
   --TODO: replace with the actual caravan entity once they move in the world
   self._sv.caravan_entity = radiant.entities.create_entity()   
   radiant.entities.set_player_id(self._sv.caravan_entity, self._sv.player_id)

   self:_spawn_caravan()
end

function SimpleCaravan:_spawn_caravan()
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
               title = self._sv._trade_info.title,
               message = message_composite,
               accepted_callback = "_on_accepted",
               declined_callback = "_on_declined",
           })

      --Make sure it times out if we don't get to it
      local wait_duration = self._sv._trade_info.expiration_timeout
      self:_create_timer(wait_duration)
   end
end

function SimpleCaravan:_make_message()
   local message_composite = self._sv._trade_info.message

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
   elseif item_data.components['stonehearth:iconic_form'] then
      local full_sized_data =  radiant.resources.load_json(item_data.components['stonehearth:iconic_form'].full_sized_entity)
      return full_sized_data.components.unit_info.name
   end
end

function SimpleCaravan:_select_trade_item(want_item_id)
   local random_trade_index = rng:get_int(1, #self._sv._trade_item_table)
   local trade_item_id = self._sv._trade_item_table[random_trade_index]
   if trade_item_id == want_item_id then
      if random_trade_index > 1 then
         trade_item_id =  self._sv._trade_item_table[random_trade_index - 1]
      else 
         trade_item_id =  self._sv._trade_item_table[random_trade_index + 1]
      end
   end
   local quantity = rng:get_int(self._sv._all_trades[trade_item_id].min, self._sv._all_trades[trade_item_id].max)
   return trade_item_id, quantity
end

--- Picks an item from the trade list that the trader wants. 
--  If the town has any of those items in its inventory, the trade will want less than
--  or equal to the nubmer of items that it can reserve
--  @returns item_id, num_desired. If either is nil, we didn't find anything we wanted
function SimpleCaravan:_select_desired_item()
   --Get our inventory all ready to query
   
   local inventory = stonehearth.inventory:get_inventory(self._sv.player_id)
   local random_want_index = rng:get_int(1, #self._sv._trade_item_table)
   for i=1, #self._sv._trade_item_table do
      local item_id = self._sv._trade_item_table[random_want_index]

      --if we picked an item that has an iconic form, we actually want to check for that
      local item_real_uri = item_id
      local data = radiant.entities.get_component_data(item_id, 'stonehearth:entity_forms')
      if data then
         item_real_uri = data.iconic_form
      end 

      local num_desired = rng:get_int(self._sv._all_trades[item_id].min, self._sv._all_trades[item_id].max)

      --TODO: add this function to the inventory tracker
      local inventory_data_for_item = inventory:get_items_of_type(item_real_uri)
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
      if random_want_index > #self._sv._trade_item_table then
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
   local banner = town:get_banner()
   local drop_origin = banner and radiant.entities.get_world_grid_location(banner)
   if not drop_origin then
      return
   end

   local uris = {}
   uris[self._sv.trade_data.caravan_has] = self._sv.trade_data.caravan_quantity

   --TODO: attach a brief particle effect to the new stuff
   radiant.entities.spawn_items(uris, drop_origin, 1, 3, { owner = self._sv.player_id })
end

--- Call this if the player rejects the trade
function SimpleCaravan:_reject_trade()
   self:_unreserve_items()
end

--- Only actually spawn the object after the user clicks OK
function SimpleCaravan:_on_accepted()
   self:_accept_trade()
   self:_stop_timer()
   radiant.events.trigger(self, 'stonehearth:dynamic_scenario:finished')
end

function SimpleCaravan:_on_declined()
   self:_reject_trade()
   self:_stop_timer()
   radiant.events.trigger(self, 'stonehearth:dynamic_scenario:finished')
end

function SimpleCaravan:_create_timer(duration)
   self._sv.timer = stonehearth.calendar:set_timer("SimpleCaravan remove bulletin", duration, radiant.bind(self, '_timer_callback'))
end

function SimpleCaravan:_timer_callback()
   if self._sv.caravan_bulletin then
      local bulletin_id = self._sv.caravan_bulletin:get_id()
      stonehearth.bulletin_board:remove_bulletin(bulletin_id)
      self:_stop_timer()
   end
end

function SimpleCaravan:_stop_timer()
   if self._sv.timer then
      self._sv.timer:destroy()
      self._sv.timer = nil
   end
end


return SimpleCaravan