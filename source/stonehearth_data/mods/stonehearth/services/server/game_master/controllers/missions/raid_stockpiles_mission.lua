local Mission = require 'services.server.game_master.controllers.missions.mission'

local RaidStockpilesMission = class()
mixin_class(RaidStockpilesMission, Mission)

function RaidStockpilesMission:initialize(ctx, info)
   assert(ctx)
   assert(ctx.enemy_location)
   assert(info)

   self._sv.party = self:_create_party(ctx, info)
   self:_start_raid(ctx)
end

function RaidStockpilesMission:_start_raid(ctx)
   local stockpile = self:_find_closest_stockpile(ctx.enemy_location, ctx.player_id)
   if stockpile then
      radiant.not_yet_implemented()
      return
   end
   self:_raise_havoc(ctx)
end

function RaidStockpilesMission:_raise_havoc(ctx)
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
