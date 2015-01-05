local Mission = require 'services.server.game_master.controllers.missions.mission'

local RaidStockpilesMission = class()
mixin_class(RaidStockpilesMission, Mission)

function RaidStockpilesMission:initialize(ctx, info)
   assert(ctx.enemy_location)

   self._sv.party = self:_create_party(ctx, info)
   self:_start_raid(ctx)
end

function RaidStockpilesMission:_start_raid(ctx)
   assert(not self._sv.command)
   local stockpile = self:_find_closest_stockpile()
   if stockpile then
      radiant.not_yet_implemented()
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
   self._sv.command = self._sv.party:create_command('guard', centroid)
                                       :set_travel_stance('defensive')
                                       :set_arrived_stance('aggressive')
                                       :go()
end

return RaidStockpilesMission
