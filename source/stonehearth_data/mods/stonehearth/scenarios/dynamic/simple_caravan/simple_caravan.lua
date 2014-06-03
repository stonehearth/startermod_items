local SimpleCaravan = class()
local rng = _radiant.csg.get_default_rng()

--TOO MANY TODOS!  In order to make this work, we also have to fix the inventory service. 
--Suspending work on this to do the other 2 scenarios, then the inventory work, then will be back. 

function SimpleCaravan.can_spawn()
   --TODO: Right now this is a combat scenario, so it can reuse the combat curve, but it really wants some other kind of curve
   --TODO: ask Chris how to satisfy all of these conditions
   --TODO: account for only having the caravan come by every N days?
   --TODO: don't send the caravan during the winter?
   --TODO: don't send the caravan when there's been a lot of violence in the area recently
   --TODO: how to account for multiplayer? Which player gets the caravan? 
   return true
end

--[[
   Simple Caravan narrative
   Every <TODO!> a caravan from your kingdom comes by your town.
   Based on its observations of your resources, it offers to make a trade.
   TODO: Make more complex caravans based on the different kingdoms; maybe different scenarios
]]

function SimpleCaravan:__init(saved_variables)
   self.__saved_variables = saved_variables
   self._sv = self.__saved_variables:get_data()

   --Account for saving
   --TODO: what happens if you've saved while the caravan alert is present but not yet dealt with? Spawn again, like a task
   --TODO: how does this change if the caravan exists in person 

   --Read in the possible trades
   self._all_trades = radiant.resources.load_json('stonehearth:scenarios:simple_caravan').trades
   self._trade_item_table = {}
   for trade_item, data in self._all_trades do
      table.insert(self._trade_item_table, trade_item)
   end
end

function SimpleCaravan:start()
   --TODO: if we actually have to put a caravan of entities into the world, they need to belong to a faction
   --TODO: see goblin_thief.json

   --TODO: for now, just spawn the caravan immediately
   self:_on_spawn_caravan()

   --self:_schedule_next_spawn()
end

--- The caravan will spawn sometime during the day, a few hours after this triggers
--  Caravans do not spawn at night, so if it's night, add some padding
--  For testing, have the caravan spawn immediately
function SimpleCaravan:_schedule_next_spawn()
    local hours_till_spawn = 0

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
   stonehearth.calendar:set_timer(hours_till_spawn..'h', function()
         self:_on_spawn_caravan()
      end)
end

function SimpleCaravan:_on_spawn_caravan()
   --TODO: Where/how do we get the player_id for the current player? 
   local player_id = "player_1"
   --local inventory_data = stonehearth.inventory:get_inventory(player_id)

   --local random_want_index = rng:get_int(1, #self._all_trades)
   --Pick something from the caravan trade list
   --Send it to the UI
   --UI picks up the player's chosen result
   --Tells this class to make the trade
end

--[[
--- Randomly pick the thing the caravan wants.
--  If the town doesn't have it, pick another one. 
function SimpleCaravan:_pick_desired_item()
   local random_want_index = rng:get_int(1, #self._trade_item_table)
   for i = 1, #self._trade_item_table do
      local target_object_uri = self._trade_item_table[random_want_index]
      local target_obj_data = self._all_trades[target_object_uri]
      local desired_num = rng:get_int(target_obj_data.min, target_obj_data.max)

   end
   local item_uri = 
end

--- Checks if a player has a certain item in a stockpile somewhere. 
--  Returns the number of that item that the player has, 0 if none. 
function SimpleCaravan:_get_player_store_of_item(item_uri)
   local inventory_data = stonehearth.inventory:get_inventory(player_id)

end

function SimpleCaravan:make_trade(trade_data)
end
]]

return SimpleCaravan