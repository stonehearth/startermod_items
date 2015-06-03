local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_rng()

local Mission = require 'services.server.game_master.controllers.missions.mission'

local RaidStockpilesMission = class()
radiant.mixin(RaidStockpilesMission, Mission)

function RaidStockpilesMission:activate()
   Mission.activate(self)
   if not self._sv.update_orders_timer and self._sv.ctx then
      --if we're loading, then update the party orders once the game is loaded
      radiant.events.listen_once(radiant, 'radiant:game_loaded', function(e)
         self:_update_party_orders()
      end)
   end
end

function RaidStockpilesMission:can_start(ctx, info)
   if not Mission.can_start(self, ctx, info) then
      return false
   end
   --Only do this if the players are hostile to each other
   if ctx.npc_player_id and ctx.player_id and 
      not stonehearth.player:are_players_hostile(ctx.npc_player_id, ctx.player_id ) then
      return false
   end

   assert(ctx)
   assert(ctx.enemy_location)
   assert(info)
   assert(ctx.npc_player_id)

   -- don't raid the enemy stockpiles if there's no room in ours
   if info.require_free_stockpile_space then
      local friendly_stockpiles = stonehearth.inventory:get_inventory(ctx.npc_player_id)
                                                            :get_all_stockpiles()
      if friendly_stockpiles then
         for _, stockpile in pairs(friendly_stockpiles) do
            local full = stockpile:get_component('stonehearth:stockpile')
                                    :is_full()
            if not full then
               return true
            end
         end
      end
      return false
   end
   return true
end

function RaidStockpilesMission:start(ctx, info)
   Mission.start(self, ctx, info)

   if info.npc_player_id then
      ctx.npc_player_id = info.npc_player_id
   end
   self:_update_party_orders()
end

function RaidStockpilesMission:stop()
   Mission.stop(self)

   if self._stockpile_listener then
      self._stockpile_listener:destroy()
      self._stockpile_listener = nil
   end
   if self._sv.update_orders_timer then
      self._sv.update_orders_timer:destroy()
      self._sv.update_orders_timer = nil
      self.__saved_variables:mark_changed()
   end
end

function RaidStockpilesMission:_update_party_orders()
   local ctx = self._sv.ctx
   local info = self._sv.info
   
   if self._sv.update_orders_timer then
      self._sv.update_orders_timer:destroy()
      self._sv.update_orders_timer = nil
      self.__saved_variables:mark_changed()
   end   

   local raid_stockpile = self:_find_closest_stockpile(ctx.enemy_location, ctx.player_id)
   if raid_stockpile then
      local friendly_stockpiles = stonehearth.inventory:get_inventory(ctx.npc_player_id)
                                                            :get_all_stockpiles()
      self:_raid_stockpile(raid_stockpile, friendly_stockpiles)

      --This means that if there are player stockpiles, but no friendly stockpiles, 
      --the thieves won't know what to do, and may wander aimlessly. The marauders, 
      --however, will raid the stockpiles. If there are no player stockpiles, 
      --we will currently pick a random point, hopefully attacking along the way.
      return
   end
   self:_pick_random_spot(ctx)
end

function RaidStockpilesMission:_raid_stockpile(raid_stockpile, friendly_stockpiles)
   self._raiding_stockpile = raid_stockpile
   if self._stockpile_listener then
      self._stockpile_listener:destroy()
      self._stockpile_listener = nil
   end
   self._stockpile_listener = radiant.events.listen(self._raiding_stockpile, 'stonehearth:stockpile:item_removed', self, self._check_stockpile)

   -- the formation should be the center of the stockpile
   local location = raid_stockpile:get_component('stonehearth:stockpile')
                                       :get_bounds()
                                          :get_centroid()   

   local party = self._sv.party
   party:place_banner(party.ATTACK, location, 0)
   party:cancel_tasks()
   
   --raid player stockpiles if there are friendly stockpiles
   if friendly_stockpiles and next(friendly_stockpiles) then
      for _, stockpile in pairs(friendly_stockpiles) do
         party:add_task('stonehearth:steal_items_in_stockpile', {
                     from_stockpile = raid_stockpile,
                     to_stockpile = stockpile,
                  })
                  :set_priority(stonehearth.constants.priorities.combat.IDLE)
                  :start()
      end
   end
   --smash player stockpiles regardless
   party:add_task('stonehearth:destroy_items_in_stockpile', {
            stockpile = raid_stockpile,
         })
         :set_priority(stonehearth.constants.priorities.combat.IDLE)
         :start()

end

function RaidStockpilesMission:_check_stockpile(e)
   if e.stockpile == self._raiding_stockpile then
      local items = e.stockpile:get_component('stonehearth:stockpile')
                                 :get_items()
      if not next(items) then
         if self._stockpile_listener then
            self._stockpile_listener:destroy()
            self._stockpile_listener = nil
         end
         self:_update_party_orders()
      end
   end 
end

function RaidStockpilesMission:_pick_random_spot(ctx)
   if self._sv.update_orders_timer then
      self._sv.update_orders_timer:destroy()
      self._sv.update_orders_timer = nil
      self.__saved_variables:mark_changed()
   end   
   self._sv.update_orders_timer = stonehearth.calendar:set_timer("RaidStockpilesMission update_party_orders", '4h', radiant.bind(self, '_update_party_orders'))
   self.__saved_variables:mark_changed()

   local town = stonehearth.town:get_town(self._sv.ctx.player_id)
   if not town then
      return
   end
   local camp_standard = town:get_banner()   
   if not camp_standard then
      return
   end
   local location = radiant.entities.get_world_grid_location(camp_standard)
   if not location then
      return
   end
   
   local location = location + Point3(rng:get_int(-25, 25), 0, rng:get_int(-25, 25))
   local party = self._sv.party
   party:place_banner(party.ATTACK, location, 0)
   party:cancel_tasks()
end

return RaidStockpilesMission
