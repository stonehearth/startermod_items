local Mission = require 'services.server.game_master.controllers.missions.mission'

local RaidStockpilesMission = class()
mixin_class(RaidStockpilesMission, Mission)

function RaidStockpilesMission:initialize(ctx, info)
   assert(ctx)
   assert(ctx.enemy_location)
   assert(info)

   self._sv.ctx = ctx
   self._sv.party = self:_create_party(ctx, info)
   self:_update_party_orders()
end

function RaidStockpilesMission:_update_party_orders()
   local ctx = self._sv.ctx
   local stockpile = self:_find_closest_stockpile(ctx.enemy_location, ctx.player_id)
   if stockpile then
      self:_raid_stockpile(stockpile)
      return
   end
   self:_attack_town_center(ctx)
end

function RaidStockpilesMission:_raid_stockpile(stockpile)
   self._raiding_stockpile = stockpile
   self._stockpile_listener = radiant.events.listen(self._raiding_stockpile, 'stonehearth:stockpile:item_removed', self, self._check_stockpile)
   local location = radiant.entities.get_world_grid_location(stockpile)
   self._sv.party:raid(stockpile)
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

function RaidStockpilesMission:_attack_town_center(ctx)
   local town = stonehearth.town:get_town(ctx.player_id)
   if not town then
      return
   end
   local centroid = town:get_centroid()
   if not centroid then
      return
   end
   self._sv.party:create_attack_order(centroid, 0)
end

return RaidStockpilesMission
